#pragma once

#include <llvm/IR/Function.h>
#include <map>
#include <vector>

#include "digraph.hh"

// Represent the Control flow grapg of a function for basic blocks
class CFG {

public:
  CFG(const llvm::Function &fun);

  // Get list of predecessors of basic block bb
  std::vector<const llvm::BasicBlock *> preds(const llvm::BasicBlock &bb) const;

  // Get list of successors of basic block bb
  std::vector<const llvm::BasicBlock *> succs(const llvm::BasicBlock &bb) const;

  // Get list of basic blocks in reverse postorder
  std::vector<const llvm::BasicBlock *> rev_postorder() const;

private:
  const llvm::Function &_fun;
  std::vector<const llvm::BasicBlock *> _bbs;
  std::map<const llvm::BasicBlock *, std::size_t> _inv;
  Digraph _graph;

  void _build_graph();
};
