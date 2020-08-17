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
  std::vector<BasicBlock *> preds(BasicBlock &bb) const;
  std::vector<const BasicBlock *> preds(const BasicBlock &bb) const;

  // Get list of successors of basic block bb
  std::vector<BasicBlock *> succs(BasicBlock &bb) const;
  std::vector<const BasicBlock *> succs(const BasicBlock &bb) const;

  // Get list of basic blocks in reverse postorder
  std::vector<const BasicBlock *> rev_postorder() const;

  const VertexAdapter<const BasicBlock *> &va() const { return _va; }

  // Get a VA for mutable blocks given a mutable bb or fun
  const VertexAdapter<BasicBlock *> &mva(Function &fun) const;
  const VertexAdapter<BasicBlock *> &mva(BasicBlock &bb) const {
    return mva(bb.parent());
  }

  const Digraph &graph() const { return _graph; }

private:
  const Function &_fun;
  const VertexAdapter<const BasicBlock *> _va;
  const Digraph _graph;
  mutable std::unique_ptr<VertexAdapter<BasicBlock *>> _mva;
};
