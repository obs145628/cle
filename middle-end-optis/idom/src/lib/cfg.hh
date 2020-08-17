#pragma once

#include <memory>

#include "digraph.hh"
#include "module.hh"
#include "vertex-adapter.hh"

// Represent the Control flow graph of a function for basic blocks
class CFG {

public:
  CFG(const Function &fun);

  // Get list of predecessors of basic block bb
  std::vector<BasicBlock *> preds(BasicBlock &bb);
  std::vector<const BasicBlock *> preds(const BasicBlock &bb) const;

  // Get list of successors of basic block bb
  std::vector<BasicBlock *> succs(BasicBlock &bb);
  std::vector<const BasicBlock *> succs(const BasicBlock &bb) const;

  // Get list of basic blocks in reverse postorder
  std::vector<const BasicBlock *> rev_postorder() const;

  const VertexAdapter<const BasicBlock *> &va() const { return _va; }

private:
  const Function &_fun;
  VertexAdapter<const BasicBlock *> _va;
  std::unique_ptr<VertexAdapter<BasicBlock *>> _mva;
  Digraph _graph;

  VertexAdapter<BasicBlock *> &_get_mva(BasicBlock &bb);

  void _build_graph();
};
