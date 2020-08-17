#pragma once

#include <llvm/IR/Module.h>
#include <memory>

#include "digraph.hh"
#include "vertex-adapter.hh"

// Represent the Control flow graph of a function for basic blocks
class CFG {

public:
  CFG(llvm::Function &fun, bool reverse = false);

  // Get list of predecessors of basic block bb
  std::vector<llvm::BasicBlock *> preds(llvm::BasicBlock &bb) const;

  // Get list of successors of basic block bb
  std::vector<llvm::BasicBlock *> succs(llvm::BasicBlock &bb) const;

  // Get list of basic blocks in postorder
  std::vector<llvm::BasicBlock *> postorder() const;

  // Get list of basic blocks in reverse postorder
  std::vector<llvm::BasicBlock *> rev_postorder() const;

  const VertexAdapter<llvm::BasicBlock *> &va() const { return _va; }

  // Underlying graph
  Digraph &graph() { return _graph; }
  const Digraph &graph() const { return _graph; }

private:
  const VertexAdapter<llvm::BasicBlock *> _va;
  Digraph _graph;

  void _build_graph();
};
