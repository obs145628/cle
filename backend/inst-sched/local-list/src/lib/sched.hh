#pragma once

#include <map>
#include <memory>
#include <utility>

#include "../isa/module.hh"
#include "../utils/digraph.hh"

#include <logia/md-gfm-doc.hh>

// Local List Scheduling
// CPUs may be able to start an instruction while another in still running
// Each CPU may start at most N CPUs by cycle
// (in this version, N is fixed at 1)
// Local List decides at which cycle to start each instruction to finish
// execution as early as possible.
// For static scheduling, can decide exactly at which cycle which instruction is
// started
// For dynamic scheduling, just reorder the list of instructions.
// This version only works to schedule a basicblock
//
// This kind of algorithm isn't really usefull for most general purpose CPUs,
// They perform Out of Order execution. OOE is implemented in hardware using
// some kinf of scheduling algorithm (with same purpose as this one)
// But they have runtime infos not available to compiler, which helps to do
// more precise scheduling
//
// Algorithm Local List Scheduling - Engineer a Compiler p651
class Scheduler {

public:
  Scheduler(isa::Module &mod);

  void run();

private:
  isa::Module &_mod;
  isa::BasicBlock *_bb;

  // Dependence graph
  // x -> y means x must be executed before y
  // In Engineer Compiler, graph order is reversed:
  // x is a leaf means x has no preds
  // x is a root means x has no succs
  std::unique_ptr<Digraph> _depg;

  std::unique_ptr<logia::MdGfmDoc> _doc;

  std::map<std::string, int> _delay;
  std::vector<std::size_t> _latencies;

  // Schedule state
  std::size_t _cycle;
  std::vector<std::size_t> _ready;
  std::vector<std::pair<std::size_t, std::size_t>> _active;
  std::vector<std::size_t> _sched; // ins id => start cycle
                                   // (or CYCLE_NONE if not scheduled yet)

  void _run(isa::BasicBlock &bb);

  void _schedule();
  void _schedule_fwd();
  void _schedule_bwd();

  void _reorder_code();

  void _init_delay();

  std::vector<std::size_t> _get_leaves();
  std::vector<std::size_t> _get_roots();

  void _compute_latencies();

  std::size_t _pop_ready();

  bool _is_completed(std::size_t ins_id);
  bool _fwd_is_ready(std::size_t ins_id);
  bool _bwd_is_ready(std::size_t ins_id);

  std::vector<std::size_t> _heur_latency(const std::vector<std::size_t> &nodes);
  std::vector<std::size_t>
  _heur_succs_count(const std::vector<std::size_t> &nodes);

  void _dump_sched(const std::string &name);
};
