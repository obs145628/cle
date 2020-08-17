#pragma once

#include <map>
#include <memory>
#include <utility>

#include "../isa/module.hh"
#include "../utils/digraph.hh"
#include "cfg.hh"
#include "eb-paths.hh"

#include <logia/md-gfm-doc.hh>

// Local List Scheduling with Extended Basic Blocks
// CPUs may be able to start an instruction while another in still running
// Each CPU may start at most N CPUs by cycle
// (in this version, N is fixed at 1)
// Local List decides at which cycle to start each instruction to finish
// execution as early as possible.
// For static scheduling, can decide exactly at which cycle which instruction is
// started
// For dynamic scheduling, just reorder the list of instructions.
// This version schedule whole paths in the CFG.
// A path is a sequence of basic block. This give more opportunities for
// improvements
// Scheduling may move an instruction out of a basic block.
// Compensation code is then required because this removed instruction may be
// needed for another path.
// Paths are found using Extend Basic Blocks
// CFG is divided into a group a EBBs, and each EBB is further divided into
// paths.
// Paths are sorted from more frequently taken to less frequently taken.
// These paths  handled firsts are less likely to have compensation code
// inserted, which makes them faster than later paths
//
// This kind of algorithm isn't really usefull for most general purpose CPUs,
// They perform Out of Order execution. OOE is implemented in hardware using
// some kinf of scheduling algorithm (with same purpose as this one)
// But they have runtime infos not available to compiler, which helps to do
// more precise scheduling
//
// Algorithm Scheduling Extended Basic Blocks - Engineer a Compiler p661
class Scheduler {

public:
  Scheduler(isa::Function &fun);

  void run();

private:
  isa::Function &_fun;
  const CFG &_cfg;
  const std::vector<const EbPaths::path_t *> &_paths;

  const EbPaths::path_t *_path;
  std::size_t _path_cut;
  isa::BasicBlock *_bb;
  std::map<const isa::BasicBlock *, std::vector<std::size_t>> _saved_scheds;

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

  void _schedule_path(const EbPaths::path_t &path);
  void _schedule();
  std::vector<std::size_t> _restore_sched();
  std::size_t _restore_cycle();
  std::vector<std::size_t> _restore_ready();
  std::vector<std::pair<std::size_t, std::size_t>> _restore_active();

  void _reorder_code();

  void _init_delay();

  std::vector<std::size_t> _get_leaves();

  void _compute_latencies();

  std::size_t _pop_ready();

  bool _is_completed(std::size_t ins_id);
  bool _is_ready(std::size_t ins_id);

  std::vector<std::size_t> _heur_latency(const std::vector<std::size_t> &nodes);
  std::vector<std::size_t>
  _heur_succs_count(const std::vector<std::size_t> &nodes);

  void _dump_sched();
};
