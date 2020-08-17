#include "allocator.hh"

#include <algorithm>
#include <logia/program.hh>
#include <utils/cli/err.hh>

#include "../utils/dyn-graph.hh"
#include "../utils/union-find.hh"
#include "coalescing.hh"
#include "interference-graph.hh"
#include "live-now.hh"
#include "live-out.hh"
#include "spill-cost.hh"

namespace {

constexpr std::size_t REG_NONE = -1;

bool is_reg(const std::string &str) { return str.size() > 1 && str[0] == '%'; }

} // namespace

Allocator::Allocator(isa::Function &fun, std::size_t hr_count)
    : _fun(fun), _ctx(_fun.parent().ctx()), _hr_count(hr_count) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>("Allocator for @" +
                                                             _fun.name());
}

void Allocator::apply(isa::Module &mod, std::size_t hr_count) {
  for (auto fun : mod.funs()) {
    Allocator alloc(*fun, hr_count);
    alloc.run();
  }
}

void Allocator::run() {
  _next_spill_pos = 0;
  _compute_live_ranges();

  // Perform coaslescing before coloring
  Coalescing coalescing(*_if_fun);
  coalescing.run();

  // Continue until finally able to color graph
  while (!_step_color())
    continue;

  _dump_assign();
  _rewrite_code();
}

void Allocator::_compute_live_ranges() {
  // Compute live-ranges for all regs
  // Because the code is in SSA, all vars are already in different live-range
  // They must be combined only for phi instructions
  UnionFind<std::string> live_ranges;

  for (auto bb : _fun.bbs())
    for (const auto &ins : bb->code()) {
      isa::Ins cins(_ctx, ins);
      for (const auto &r : cins.args_uses())
        if (r != "sp")
          live_ranges.connect(r, r);

      for (const auto &r : cins.args_defs())
        if (r != "sp")
          live_ranges.connect(r, r);

      if (cins.opname() == "phi") {
        // Connect the def and all uses together
        auto d = cins.args_defs().at(0);
        for (const auto &u : cins.args_uses())
          live_ranges.connect(d, u);
      }
    }

  live_ranges.compress();

  *_doc << "## Live Ranges Computation\n";

  for (std::size_t i = 0; i < live_ranges.sets_count(); ++i) {
    *_doc << "- `LR" << i << ": {";
    auto lr = live_ranges.get_set(i);
    for (const auto &r : lr)
      *_doc << r << "; ";
    *_doc << "}`\n";
  }

  // Convert module to LiveRange form
  // (Replace all regs with the corresponding LiveRange)
  _if_mod = _fun.parent().clone();
  _if_fun = _if_mod->funs().front();

  for (auto bb : _if_fun->bbs()) {

    // First remove all phi instructions
    for (std::size_t i = 0; i < bb->code().size(); ++i)
      if (bb->code()[i][0] == "phi")
        bb->code().erase(bb->code().begin() + i--);

    for (auto &ins : bb->code())
      for (auto &arg : ins) {
        if (arg == "%sp" || !is_reg(arg))
          continue;
        auto r = arg.substr(1);
        auto lr = live_ranges.find(r);
        arg = "%lr" + std::to_string(lr);
      }
  }

  *_doc << "## Live Ranges Code\n";
  _if_fun->dump_code(*_doc);
}

bool Allocator::_step_color() {
  // Run one step of color algorithm
  // Return true if color was successfull,
  // otherwhise spill some regs and returns false

  *_doc << "## RegAlloc round\n";

  // Convert interference graph into dynamic graph
  const auto &ig = _if_fun->get_analysis<InterferenceGraph>().graph();
  DynGraph<std::size_t> dyn_ig;
  for (std::size_t u = 0; u < ig.v(); ++u)
    dyn_ig.add_vertex(u);
  for (std::size_t u = 0; u < ig.v(); ++u)
    for (std::size_t v = 0; v < ig.v(); ++v)
      if (ig.has_edge(u, v))
        dyn_ig.add_edge(u, v);

  std::vector<std::size_t> lr_stack;

  // Build stack of live ranges, vertices will try to be colored in reverse
  // order
  while (dyn_ig.v()) {
    // try to take an unconstrained reg first
    std::size_t next = REG_NONE;
    for (auto v : dyn_ig.vertices())
      if (dyn_ig.degree(v) < _hr_count) {
        next = v;
        break;
      }

    // if all are constrained, choose one using an heuristic
    if (next == REG_NONE)
      next = _choose_lr_alloc(dyn_ig.vertices());

    lr_stack.push_back(next);
    dyn_ig.del_vertex(next);
  }

  *_doc << "### Assignments\n";
  // Set all live-ranges to non assignged
  _assignments.assign(ig.v(), REG_NONE);

  // Rebuild graph and color nodes
  while (!lr_stack.empty()) {
    auto lr = lr_stack.back();
    lr_stack.pop_back();

    // Add lr and conresponding edges to the graph
    dyn_ig.add_vertex(lr);
    for (auto neigh : ig.neighs(lr))
      if (dyn_ig.has_vertex(neigh))
        dyn_ig.add_edge(lr, neigh);

    auto reg = _try_assign(lr, dyn_ig);
    if (reg == REG_NONE) {
      // failed
      *_doc << "- Failed to assign live range %lr" << lr << "\n";
      _spill(lr);
      return false;
    }

    _assignments[lr] = reg;
    *_doc << " - Assigned `%lr" << lr << "` to register `%hr" << reg << "`\n";
  }

  return true;
}

std::size_t Allocator::_choose_lr_alloc(const std::vector<std::size_t> &lrs) {
  // Choose among lives ranges in lrs, which to select for allocation
  // heuristic based on spill metric
  // choose lr with minimal spill cost
  const auto &sc = _if_fun->get_analysis<SpillCost>();
  std::size_t best = REG_NONE;
  double best_cost = SPILL_COST_INF;

  for (auto &lr : lrs) {
    auto cost = sc.lr_cost(lr);
    if (cost <= best_cost) {
      best = lr;
      best_cost = cost;
    }
  }

  assert(best != REG_NONE);
  return best;
}

std::size_t Allocator::_try_assign(std::size_t lr,
                                   const DynGraph<std::size_t> &ig) {
  // Try to assign a register to lr
  // Take a color not taken by any other neighbor

  std::set<std::size_t> free_regs;
  for (std::size_t i = 0; i < _hr_count; ++i)
    free_regs.insert(i);

  for (auto n : ig.neighs(lr))
    if (_assignments[n] != REG_NONE)
      free_regs.erase(_assignments[n]);

  return free_regs.empty() ? REG_NONE : *free_regs.begin();
}

void Allocator::_spill(std::size_t lr) {
  // spill lr
  // assign it a stack position
  // then for all uses, replace it with a load to a new live range
  // and for all defs, replace it with a store from a new live range
  // create new live ranges for all, except for first

  const auto &sc = _if_fun->get_analysis<SpillCost>();
  auto lr_arg = "%lr" + std::to_string(lr);
  std::size_t next_lr = lr;

  if (sc.lr_cost(lr) == SPILL_COST_INF) {
    _doc = nullptr;
    PANIC("Ttrying to spill reg with infinite cost");
  }

  // stack may be already used by the code
  // figure out what stack value is available
  if (_next_spill_pos == 0)
    _next_spill_pos = _get_first_spill_pos();
  std::size_t pos = _next_spill_pos;
  _next_spill_pos += 4;

  for (auto bb : _if_fun->bbs())
    for (std::size_t ins_idx = 0; ins_idx < bb->code().size(); ++ins_idx) {
      auto &ins = bb->code()[ins_idx];
      isa::Ins cins(_ctx, ins);
      std::vector<std::string> pre_spill;
      std::vector<std::string> post_spill;
      std::size_t use_lr = REG_NONE;

      for (std::size_t i = 0; i < cins.args().size() - 1; ++i) {
        bool need_next = false;

        if (cins.get_arg_kind(i) == isa::ARG_KIND_USE && ins[i + 1] == lr_arg) {
          if (use_lr == REG_NONE) {
            // only load once if lr used multiple times
            use_lr = next_lr;
            need_next = true;
            pre_spill = {"load", "%lr" + std::to_string(use_lr), "%sp",
                         std::to_string(pos)};
          }

          ins[i + 1] = "%lr" + std::to_string(use_lr);
        }

        else if (cins.get_arg_kind(i) == isa::ARG_KIND_DEF &&
                 ins[i + 1] == lr_arg) {
          auto def_lr = next_lr;
          need_next = true;
          post_spill = {"store", "%lr" + std::to_string(def_lr), "%sp",
                        std::to_string(pos)};
          ins[i + 1] = "%lr" + std::to_string(def_lr);
        }

        if (need_next) {
          // computed inded of next creatd lr
          next_lr = (next_lr == lr) ? sc.lr_count() : next_lr + 1;
        }
      }

      // Insert spill code
      if (!post_spill.empty())
        bb->code().insert(bb->code().begin() + ins_idx + 1, post_spill);
      if (!pre_spill.empty())
        bb->code().insert(bb->code().begin() + ins_idx, pre_spill);
      ins_idx += !pre_spill.empty();
      ins_idx += !post_spill.empty();
    }

  // All analyses need to be computed again.
  _if_fun->invalidate_analysis<InterferenceGraph>();
  _if_fun->invalidate_analysis<LiveOut>();
  _if_fun->invalidate_analysis<LiveNow>();
  _if_fun->invalidate_analysis<SpillCost>();

  *_doc << "- live range `%lr" << lr << "` spilled to `%sp + " << pos << "`\n";
  *_doc << "### Updated code\n";
  _if_fun->dump_code(*_doc);
}

std::size_t Allocator::_get_first_spill_pos() {
  // Find what is the highest value used by stack
  std::size_t max_pos = 0;

  for (auto bb : _if_fun->bbs())
    for (const auto &ins : bb->code())
      if ((ins[0] == "load" || ins[0] == "store") && ins[2] == "%sp")
        max_pos = std::max(max_pos, std::size_t(std::atoi(ins[3].c_str())));

  return max_pos + 4;
}

void Allocator::_dump_assign() {
  *_doc << "## Final assignment:\n";
  for (std::size_t i = 0; i < _assignments.size(); ++i)
    *_doc << "- `%lr" << i << " => %hr" << _assignments[i] << "`\n";
}

void Allocator::_rewrite_code() {
  for (std::size_t i = 0; i < _fun.bbs().size(); ++i) {
    // Code is same as live-range version
    auto &out_bb = *_fun.bbs()[i];
    out_bb.code() = _if_fun->bbs()[i]->code();

    // But replace live-range with assigned hard regs
    for (auto &ins : out_bb.code())
      for (auto &r : ins) {
        if (!is_reg(r) || r == "%sp")
          continue;
        auto lr = std::atoi(r.c_str() + 3);
        auto hr = _assignments.at(lr);
        assert(hr != REG_NONE);
        r = "%hr" + std::to_string(hr);
      }
  }

  *_doc << "## Rewriten code\n";
  _fun.dump_code(*_doc);
}
