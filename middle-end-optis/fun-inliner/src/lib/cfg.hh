#pragma once

#include <map>
#include <vector>

#include "digraph.hh"
#include "module.hh"

// Represent the CFG of a function for basic blocks
// It need to be rebuild every time a bb is added / deleted, or a terminator ins
// is changed
class CFG {

public:
  CFG(Function &fun);

  // Get list of predecessors of basic block bb
  std::vector<BasicBlock *> preds(BasicBlock &bb) const;

  // Get list of successors of basic block bb
  std::vector<BasicBlock *> succs(BasicBlock &bb) const;

private:
  Function &_fun;
  std::vector<BasicBlock *> _bbs;
  std::map<BasicBlock *, std::size_t> _inv;
  Digraph _graph;

  void _build_graph();
};

// Build the Control Flow Graph of a function
// G has n vertices, one for each basic block
// There is an edge from u to v if last instruction of bb u may branch to bb v
Digraph build_cfg(const Function &fun);
