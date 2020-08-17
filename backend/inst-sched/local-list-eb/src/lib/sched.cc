#include "sched.hh"

#include <algorithm>
#include <cassert>
#include <fstream>

#include "dep.hh"
#include "ebb-view.hh"
#include "renamer.hh"
#include <logia/program.hh>
#include <utils/str/str.hh>

#define DELAY_PATH (CMAKE_SRC_DIR "/config/delay.txt")

namespace {

constexpr std::size_t CYCLE_NONE = -1;

void log_fun(const std::string &title, const isa::Function &f) {
  auto doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(title);
  auto ch = doc->code("asm");
  f.dump_code(std::cout);
  f.dump_code(ch.os());
}

std::ostream &operator<<(std::ostream &os,
                         const std::vector<std::string> &ins) {
  os << "    ";
  for (std::size_t i = 0; i < ins.size(); ++i) {
    if (i == 1)
      os << " ";
    else if (i > 1)
      os << ", ";
    os << ins[i];
  }
  return os;
}

} // namespace

Scheduler::Scheduler(isa::Function &fun)
    : _fun(fun), _cfg(_fun.get_analysis<CFG>()),
      _paths(_fun.get_analysis<EbPaths>().paths()) {}

void Scheduler::run() {
  // Load static informations needed for scheduling
  _init_delay();

  // Schedule all paths one
  for (auto p : _paths)
    _schedule_path(*p);

  log_fun("Scheduled IR: @" + _fun.name(), _fun);
  _fun.check();
}

void Scheduler::_schedule_path(const EbPaths::path_t &path) {
  assert(!path.empty());

  std::string title = "Schedule EBB @" + path[0]->parent().name() + ": (";
  for (std::size_t i = 0; i < path.size(); ++i) {
    title += path[i]->name();
    if (i + 1 != path.size())
      title += ", ";
  }
  title += ")";
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(title);
  _path = &path;
  // _path_cut is the index in _path of the first non-scheduled bb
  _path_cut = 0;
  while (_saved_scheds.count(path.at(_path_cut)))
    ++_path_cut;

  // Build dependency graph
  _depg = std::make_unique<Digraph>(make_dep_graph(path));

  // Some dependencies are added between terminal and next ins
  // to avoid code moving to a previous bb
  // These dependencies can be removed for bbs already scheduled
  EbbView ebb(*_path);
  for (std::size_t i = 0; i < _path_cut; ++i) {
    auto term_idx = ebb.offset_of(*(*_path)[i + 1]) - 1;
    for (auto s : _depg->succs(term_idx))
      _depg->del_edge(term_idx, s);
  }

  // Assign priorites to each instruction
  _compute_latencies();

  // Schedule all instructions
  _schedule();

  // Reorder instructions order using computed schedule
  _reorder_code();

  _doc = nullptr;
}

// Run both forward and backward scheduling
// Take the best result of both
void Scheduler::_schedule() {
  _sched = _restore_sched();
  _cycle = _restore_cycle();
  _ready = _restore_ready();
  _active = _restore_active();
  EbbView ebb(*_path);

  while (!_ready.empty() || !_active.empty()) {

    // Find all instructions that finished executing at this cycle
    // Remove them from active
    // Add all of its ready succs to ready
    for (std::size_t i = 0; i < _active.size(); ++i) {
      if (_cycle < _active[i].second)
        continue;
      assert(_active[i].second == _cycle);
      auto ins_id = _active[i].first;

      for (auto s : _depg->succs(ins_id))
        if (_is_ready(s) &&
            std::find(_ready.begin(), _ready.end(), s) == _ready.end())
          _ready.push_back(s);

      _active.erase(_active.begin() + i--);
    }

    // Schedule an instruction if available
    if (!_ready.empty()) {
      auto ins_id = _pop_ready();
      const auto &ins = ebb[ins_id];
      _sched[ins_id] = _cycle;
      _active.emplace_back(ins_id, _cycle + _delay.at(ins[0]));
    }

    ++_cycle;
  }

  // Fix cycle count
  // -1 because went one cycle after end
  // -1 because cycle is an index, not a count
  _cycle = _cycle - 2;

  _dump_sched();
}

std::vector<std::size_t> Scheduler::_restore_sched() {
  EbbView ebb(*_path);
  std::vector<std::size_t> res(ebb.size(), CYCLE_NONE);
  if (_path_cut == 0)
    return res;

  const auto &saved = _saved_scheds.at((*_path)[_path_cut - 1]);
  for (std::size_t i = 0; i < saved.size(); ++i)
    res[i] = saved[i];
  return res;
}

std::size_t Scheduler::_restore_cycle() {
  if (_path_cut == 0)
    return 1;

  const auto &saved = _saved_scheds.at((*_path)[_path_cut - 1]);
  return saved.back() + 1;
}

std::vector<std::size_t> Scheduler::_restore_ready() {
  if (_path_cut == 0)
    return _get_leaves();

  std::vector<std::size_t> res;
  EbbView ebb(*_path);

  for (std::size_t i = 0; i < ebb.size(); ++i)
    if (_is_ready(i))
      res.push_back(i);
  return res;
}

std::vector<std::pair<std::size_t, std::size_t>> Scheduler::_restore_active() {
  if (_path_cut == 0)
    return {};

  std::vector<std::pair<std::size_t, std::size_t>> res;
  EbbView ebb(*_path);

  for (std::size_t i = 0; i < ebb.size(); ++i) {
    if (_sched[i] == CYCLE_NONE)
      continue;
    std::size_t end = _cycle + _delay.at(ebb[i][0]);
    if (end > _cycle)
      res.emplace_back(i, end);
  }

  return res;
}

void Scheduler::_reorder_code() {
  // First check if all ins where scheduled
  for (auto x : _sched)
    assert(x != CYCLE_NONE);

  // Only write code for new basic blocks
  auto sched = _sched;
  auto path = *_path;
  for (std::size_t i = 0; i < _path_cut; ++i) {
    sched.erase(sched.begin(), sched.begin() + path.front()->code().size());
    path.erase(path.begin());
  }
  EbbView ebb(path);
  assert(ebb.size() == sched.size());

  // Build new list of code
  using ins_t = std::vector<std::string>;
  using code_t = std::vector<ins_t>;
  code_t new_bb;
  std::vector<code_t> new_code;
  std::vector<std::size_t> sched_ordered;
  auto bb_it = path.begin();

  // Restore sched ordered if previous paths scheduled
  if (_path_cut != 0)
    sched_ordered = _saved_scheds.at((*_path)[_path_cut - 1]);

  std::map<const isa::BasicBlock *, code_t> moved_out;

  for (std::size_t i = 0; i < sched.size(); ++i) {
    // Find ins with smallest cycle
    std::size_t next_ins = 0;
    for (std::size_t ins = 0; ins < sched.size(); ++ins)
      if (sched[ins] < sched[next_ins])
        next_ins = ins;

    // Add this ins to code
    new_bb.push_back(ebb[next_ins]);
    sched_ordered.push_back(sched[next_ins]);
    sched[next_ins] = CYCLE_NONE;

    // Keep track of code that was moved to another bb
    auto ins_bb = &ebb.bb_of(next_ins);
    if (ins_bb != *bb_it)
      moved_out[ins_bb].push_back(ebb[next_ins]);

    // Finish bb if ins is a terminal
    isa::Ins cins(ebb.ctx(), ebb[next_ins]);
    if (cins.is_term()) {
      new_code.push_back(new_bb);
      new_bb.clear();

      // Need to save generated sched to use for future paths starting with
      // bbs already scheduled
      _saved_scheds[*bb_it] = sched_ordered;

      ++bb_it;
    }
  }

  // Update basic blocks in path
  assert(new_code.size() == path.size());
  for (std::size_t i = 0; i < new_code.size(); ++i) {
    auto bb = const_cast<isa::BasicBlock *>(path[i]);
    bb->code() = new_code[i];
  }

  // Some code may have been moved from BB x to BB y
  // It must be moved at the beginning of all blocks z such that there is an
  // edge x->z and z isn't the block next to x in the path
  for (auto out_it : moved_out) {
    auto x = out_it.first;
    auto bb_it = std::find(path.begin(), path.end(), x);
    assert(bb_it != path.end());
    ++bb_it;
    assert(bb_it != path.end()); // ins cannot be moved out of last bb
    for (auto succ : _cfg.succs(*x)) {
      if (succ == *bb_it)
        continue;
      auto z = const_cast<isa::BasicBlock *>(succ);
      assert(_saved_scheds.count(z) == 0);

      z->code().insert(z->code().begin(), out_it.second.begin(),
                       out_it.second.end());
      *_doc << "## Move code: " << x->name() << " -> " << z->name() << "\n";
      z->dump_code(*_doc);
    }
  }

  // LiveOut will need to be recomputed
  _fun.invalidate_analysis<LiveOut>();

  *_doc << "## Scheduled code\n";
  for (auto bb : path)
    bb->dump_code(*_doc);
}

void Scheduler::_init_delay() {
  std::ifstream is(DELAY_PATH);
  std::string l;
  while (std::getline(is, l)) {
    l = utils::str::trim(l);
    if (l.empty())
      continue;

    auto args = utils::str::split(l, ' ');
    assert(args.size() == 2);
    auto key = utils::str::trim(args[0]);
    auto val = utils::str::trim(args[1]);
    _delay[key] = std::atoi(val.c_str());
  }

  _doc =
      logia::Program::instance().add_doc<logia::MdGfmDoc>("Instruction delays");
  {
    auto ch = _doc->code();
    for (auto it : _delay)
      ch << it.first << ": " << it.second << "\n";
  }
  _doc = nullptr;
}

// All nodes that have no predecessors
// Instructions that can be executed at the start
std::vector<std::size_t> Scheduler::_get_leaves() {
  std::vector<std::size_t> res;
  for (std::size_t i = 0; i < _depg->v(); ++i)
    if (_depg->preds_count(i) == 0)
      res.push_back(i);
  return res;
}

// The latency of op x is the longest distance as a delay from x to any root
void Scheduler::_compute_latencies() {
  EbbView ebb(*_path);
  _latencies.assign(ebb.size(), 1000000);

  // Reverse post-order of reverse-graph needed to make sure latency of
  // succesors are computed first
  // But because instructions are already ordered in EBB, instead can just go
  // through instructions from bottom to top

  for (std::size_t i = ebb.size() - 1; i < ebb.size(); --i) {
    std::size_t lat = 0;
    for (auto s : _depg->succs(i))
      lat = std::max(_latencies[s], lat);
    _latencies[i] = lat + _delay.at(ebb[i][0]);
  }

  *_doc << "## Latencies\n";
  {
    auto ch = _doc->code("asm");
    for (std::size_t i = 0; i < ebb.size(); ++i)
      ch << ebb[i] << " ; lat = " << _latencies.at(i) << "\n";
  }

  *_doc << "## Dependence Graph\n";
  _depg->dump_tree(*_doc);
}

// Returns an instruction from ready list
// It will be scheduled to this cycle
// The function should use a list of heuristics to decide which ins to select
// (if there is a tie for 1st heur, use 2nd, and so on)
//
// Heuristics used here are:
// - Take node with longest latency
//    (ensures always takes node in critical path)
// - Take node with largest number of successors in dep graph
std::size_t Scheduler::_pop_ready() {
  auto best = _heur_latency(_ready);
  if (best.size() > 1)
    best = _heur_succs_count(best);

  auto res = best.front();

  auto it = std::find(_ready.begin(), _ready.end(), res);
  assert(it != _ready.end());
  _ready.erase(it);
  return res;
}

// Return true if instruction was scheduled and finished
bool Scheduler::_is_completed(std::size_t ins_id) {
  EbbView ebb(*_path);
  const auto &ins = ebb[ins_id];
  return _sched[ins_id] != CYCLE_NONE &&
         _sched[ins_id] + _delay.at(ins[0]) <= _cycle;
}

// An instruction is ready if it wasn't scheduled before, and all its
// predecessors are completed
bool Scheduler::_is_ready(std::size_t ins_id) {
  if (_sched[ins_id] != CYCLE_NONE)
    return false;

  for (auto p : _depg->preds(ins_id))
    if (!_is_completed(p))
      return false;

  return true;
}

std::vector<std::size_t>
Scheduler::_heur_latency(const std::vector<std::size_t> &nodes) {
  std::vector<std::size_t> best;
  std::size_t best_val = 0;

  for (auto ins : nodes) {
    auto val = _latencies.at(ins);
    if (best.empty() || val > best_val) {
      best_val = val;
      best = {ins};
    } else if (val == best_val)
      best.push_back(ins);
  }

  return best;
}

std::vector<std::size_t>
Scheduler::_heur_succs_count(const std::vector<std::size_t> &nodes) {
  std::vector<std::size_t> best;
  std::size_t best_val = 0;

  for (auto ins : nodes) {
    auto val = _depg->succs_count(ins);
    if (best.empty() || val > best_val) {
      best_val = val;
      best = {ins};
    } else if (val == best_val)
      best.push_back(ins);
  }

  return best;
}

void Scheduler::_dump_sched() {
  EbbView ebb(*_path);
  *_doc << "## Forward Schedule (" << _cycle << " cycles)\n";
  auto ch = _doc->code("asm");

  for (std::size_t i = 0; i < ebb.size(); ++i)
    ch << ebb[i] << " ; cycle: " << _sched[i] << "\n";
}
