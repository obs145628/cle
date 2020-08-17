#pragma once

#include <vector>

#include "cfg.hh"

class DomTree {

public:
  DomTree(llvm::Function &fun, const CFG &cfg);

  const CFG &cfg() const { return _cfg; }

  llvm::BasicBlock &root() const { return *_root; }

  llvm::BasicBlock &idom(llvm::BasicBlock &bb) const;

  std::vector<llvm::BasicBlock *> dom(llvm::BasicBlock &bb) const;

private:
  llvm::Function &_fun;
  const CFG &_cfg;
  const VertexAdapter<llvm::BasicBlock *> &_va;

  llvm::BasicBlock *_root;
  std::vector<std::size_t> _idom;
  std::vector<llvm::BasicBlock *> _rpo;
  std::vector<std::size_t> _rpo_pos;

  void _run();
  void _init();
  bool _iterate();
  std::size_t _intersect(std::size_t i, std::size_t j);
  void _dump(std::size_t niter);
  void _build_dom_tree();

  llvm::BasicBlock &_find_root();
};
