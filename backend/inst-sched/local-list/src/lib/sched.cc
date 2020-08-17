#include "sched.hh"

#include <algorithm>
#include <cassert>
#include <fstream>

#include "dep.hh"
#include "renamer.hh"
#include <logia/program.hh>
#include <utils/str/str.hh>

#define DELAY_PATH (CMAKE_SRC_DIR "/config/delay.txt")

namespace {

constexpr std::size_t CYCLE_NONE = -1;

void log_mod(const std::string &title, const isa::Module &m) {
  auto doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(title);
  auto ch = doc->code("asm");
  m.dump_code(std::cout);
  m.dump_code(ch.os());
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

Scheduler::Scheduler(isa::Module &mod) : _mod(mod) {}

void Scheduler::run() {
  log_mod("Input IR", _mod);
  _mod.check();

  // Step 0 : Load static informations needed for scheduling
  _init_delay();

  // Step 1: Rename registers to avoid anti-dependencies
  Renamer renamer(_mod);
  renamer.run();
  log_mod("After Renaming", _mod);
  _mod.check();

  // schedule each bb
  for (auto fun : _mod.funs())
    for (auto bb : fun->bbs())
      _run(*bb);

  log_mod("Scheduled IR", _mod);
  _mod.check();
}

void Scheduler::_run(isa::BasicBlock &bb) {
  _bb = &bb;
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(
      "Schedule @" + bb.parent().name() + ":@" + bb.name());

  // Step 2 : Build dependence graph
  _depg = std::make_unique<Digraph>(make_dep_graph(bb));

  // Step 3 : Assign priorites to each instruction
  _compute_latencies();

  // Step 4 : schedule all instructions
  _schedule();

  // Step 5 : reorder instructions order using computed schedule
  _reorder_code();

  _doc = nullptr;
}

// Run both forward and backward scheduling
// Take the best result of both
void Scheduler::_schedule() {
  _schedule_fwd();
  auto fwd_sched = _sched;
  auto fwd_cycles = _cycle;

  _schedule_bwd();

  if (fwd_cycles <= _cycle)
    _sched = fwd_sched;
}

void Scheduler::_schedule_fwd() {
  _cycle = 1;
  _ready = _get_leaves();
  _active.clear();
  _sched.assign(_bb->code().size(), CYCLE_NONE);

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
        if (_fwd_is_ready(s) &&
            std::find(_ready.begin(), _ready.end(), s) == _ready.end())
          _ready.push_back(s);

      _active.erase(_active.begin() + i--);
    }

    // Schedule an instruction if available
    if (!_ready.empty()) {
      auto ins_id = _pop_ready();
      const auto &ins = _bb->code()[ins_id];
      _sched[ins_id] = _cycle;
      _active.emplace_back(ins_id, _cycle + _delay.at(ins[0]));
    }

    ++_cycle;
  }

  // Fix cycle count
  // -1 because went one cycle after end
  // -1 because cycle is an index, not a count
  _cycle = _cycle - 2;

  _dump_sched("Forward");
}

void Scheduler::_schedule_bwd() {
  _cycle = 1;
  _ready = _get_roots();
  _active.clear();
  _sched.assign(_bb->code().size(), CYCLE_NONE);

  while (!_ready.empty() || !_active.empty()) {

    // Find all instructions that may become ready at this cycle
    // Remove them from active
    // Add it to ready if all it's succs start after this one finishes
    for (std::size_t i = 0; i < _active.size(); ++i) {
      if (_cycle < _active[i].second)
        continue;
      assert(_active[i].second == _cycle);
      auto ins_id = _active[i].first;

      if (_bwd_is_ready(ins_id) &&
          std::find(_ready.begin(), _ready.end(), ins_id) == _ready.end())
        _ready.push_back(ins_id);

      _active.erase(_active.begin() + i--);
    }

    // Schedule an instruction if available
    if (!_ready.empty()) {
      auto ins_id = _pop_ready();
      _sched[ins_id] = _cycle;
      // Add all its preds to active
      for (auto p : _depg->preds(ins_id)) {
        const auto &ins = _bb->code()[p];
        _active.emplace_back(p, _cycle + _delay.at(ins[0]));
      }
    }

    ++_cycle;
  }

  // Schedule values are inverted
  // Put them back in right order
  std::size_t end_cycle = _cycle;
  std::size_t last_cycle = 0;
  for (std::size_t i = 0; i < _sched.size(); ++i) {
    auto &s = _sched[i];
    const auto &ins = _bb->code()[i];
    assert(s != CYCLE_NONE);
    s = end_cycle - s;
    last_cycle = std::max(last_cycle, s + _delay.at(ins[0]));
  }

  // Fix cycles count
  _cycle = last_cycle - 1;

  _dump_sched("Backward");
}

void Scheduler::_reorder_code() {

  // First check if all ins where scheduled
  for (auto x : _sched)
    assert(x != CYCLE_NONE);

  std::vector<std::vector<std::string>> new_code;

  auto sched = _sched;

  for (std::size_t i = 0; i < sched.size(); ++i) {
    // Find ins with smallest cycle
    std::size_t next_ins = 0;
    for (std::size_t ins = 0; ins < sched.size(); ++ins)
      if (sched[ins] < sched[next_ins])
        next_ins = ins;

    // Add this ins to code
    new_code.push_back(_bb->code()[next_ins]);
    sched[next_ins] = CYCLE_NONE;
  }

  _bb->code() = new_code;

  *_doc << "## Scheduled code\n";
  _bb->dump_code(*_doc);
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

// All nodes that have no successors
// Instructions that can be executed after any other ins
std::vector<std::size_t> Scheduler::_get_roots() {
  std::vector<std::size_t> res;
  for (std::size_t i = 0; i < _depg->v(); ++i)
    if (_depg->succs_count(i) == 0)
      res.push_back(i);
  return res;
}

// The latency of op x is the longest distance as a delay from x to any root
void Scheduler::_compute_latencies() {
  _latencies.assign(_bb->code().size(), 1000000);

  // Reverse post-order of reverse-graph needed to make sure latency of
  // succesors are computed first
  // But because instructions are already ordered in BB, instead can just go
  // through instructions from bottom to top

  for (std::size_t i = _bb->code().size() - 1; i < _bb->code().size(); --i) {
    std::size_t lat = 0;
    for (auto s : _depg->succs(i))
      lat = std::max(_latencies[s], lat);
    _latencies[i] = lat + _delay.at(_bb->code()[i][0]);
  }

  *_doc << "## Latencies\n";
  for (std::size_t i = 0; i < _bb->code().size(); ++i)
    *_doc << _bb->code()[i] << " ; lat = " << _latencies.at(i) << "\n";

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
  const auto &ins = _bb->code()[ins_id];
  return _sched[ins_id] != CYCLE_NONE &&
         _sched[ins_id] + _delay.at(ins[0]) <= _cycle;
}

// An instruction is ready if it wasn't scheduled before, and all its
// predecessors are completed
bool Scheduler::_fwd_is_ready(std::size_t ins_id) {
  if (_sched[ins_id] != CYCLE_NONE)
    return false;

  for (auto p : _depg->preds(ins_id))
    if (!_is_completed(p))
      return false;

  return true;
}

// An instruction is ready if it wasn't scheduled before, and all its
// successors are scheduled after the moment the ins will finish
bool Scheduler::_bwd_is_ready(std::size_t ins_id) {
  if (_sched[ins_id] != CYCLE_NONE)
    return false;

  const auto &ins = _bb->code()[ins_id];
  auto end_cycle = _cycle - _delay.at(ins[0]);

  for (auto s : _depg->succs(ins_id))
    if (_sched[s] == CYCLE_NONE || end_cycle < _sched[s])
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

void Scheduler::_dump_sched(const std::string &name) {
  *_doc << "## " << name << " Schedule (" << _cycle << " cycles)\n";
  auto ch = _doc->code("asm");

  for (std::size_t i = 0; i < _bb->code().size(); ++i)
    ch << _bb->code()[i] << " ; cycle: " << _sched[i] << "\n";
}
