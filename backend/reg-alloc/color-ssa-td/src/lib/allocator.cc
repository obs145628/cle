#include "allocator.hh"

#include <algorithm>
#include <logia/program.hh>
#include <utils/cli/err.hh>

#include "../utils/union-find.hh"
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

  // Constrained and unconstrained live ranges
  // lr is constrained if number of neighbors >= k in interference graph
  // lr_cons sorted by increasing spill cost
  // but put regs with infinite spill cost first (should never be spilled)
  // lr_uncons not sorted
  std::vector<std::size_t> lr_cons;
  std::vector<std::size_t> lr_uncons;

  const auto &ig = _if_fun->get_analysis<InterferenceGraph>().graph();
  const auto &sc = _if_fun->get_analysis<SpillCost>();

  for (std::size_t lr = 0; lr < ig.v(); ++lr) {
    if (ig.degree(lr) >= _hr_count)
      lr_cons.push_back(lr);
    else
      lr_uncons.push_back(lr);
  }

  std::sort(lr_cons.begin(), lr_cons.end(),
            [&sc](const auto &lhs, const auto &rhs) {
              auto ca = sc.lr_cost(lhs);
              auto cb = sc.lr_cost(rhs);
              if (ca == SPILL_COST_INF)
                return true;
              else if (cb == SPILL_COST_INF)
                return false;
              else
                return ca < cb;
            });

  *_doc << "### LiveRanges organization\n";
  *_doc << "- Constrained: `";
  for (const auto &r : lr_cons)
    *_doc << "%lr" << r << "; ";
  *_doc << "`\n";
  *_doc << "- Unconstrained: `";
  for (const auto &r : lr_uncons)
    *_doc << "%lr" << r << "; ";
  *_doc << "`\n";

  // Set all live-ranges to non assignged
  _assignments.assign(sc.lr_count(), REG_NONE);

  *_doc << "### Assignments\n";

  // First try to assign all constrained live ranges
  for (auto lr : lr_cons) {
    auto reg = _try_assign(lr);
    if (reg == REG_NONE) {
      // failed
      *_doc << "- Failed to assign live range %lr" << lr << "\n";
      _spill(lr);
      return false;
    }

    _assignments[lr] = reg;
    *_doc << " - Assigned `%lr" << lr << "` to register `%hr" << reg << "`\n";
  }

  // Then assign all unconstrained regs
  for (auto lr : lr_uncons) {
    auto reg = _try_assign(lr);
    assert(reg != REG_NONE); // assign unconstrained can never fail

    _assignments[lr] = reg;
    *_doc << " - Assigned `%lr" << lr << "` to register `%hr" << reg << "`\n";
  }

  return true;
}

std::size_t Allocator::_try_assign(std::size_t lr) {
  // Try to assign a register to lr
  // Take a color not taken by any other neighbor

  std::set<std::size_t> free_regs;
  for (std::size_t i = 0; i < _hr_count; ++i)
    free_regs.insert(i);

  const auto &ig = _if_fun->get_analysis<InterferenceGraph>().graph();
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
