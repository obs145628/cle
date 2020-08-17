#pragma once

#include "digraph.hh"
#include "module.hh"
#include "vertex-adapter.hh"

// Represent the Control flow graph of a function for basic blocks
class CFG {

public:
  CFG(const Function &fun);

  // Get list of predecessors of basic block bb
  std::vector<const BasicBlock *> preds(const BasicBlock &bb) const;

  // Get list of successors of basic block bb
  std::vector<const BasicBlock *> succs(const BasicBlock &bb) const;

  // Get list of basic blocks in reverse postorder
  std::vector<const BasicBlock *> rev_postorder() const;

private:
  const Function &_fun;
  VertexAdapter<const BasicBlock *> _va;
  Digraph _graph;

  void _build_graph();
};
