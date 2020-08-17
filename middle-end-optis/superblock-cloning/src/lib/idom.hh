#pragma once

#include "cfg.hh"
#include "module.hh"

// Represent the Dominance tree
class IDom {
public:
  IDom(const Function &fun, const CFG &cfg);

  const BasicBlock &root() const;

  // Return immediate dominator of bb
  // Panic if root
  BasicBlock &idom(BasicBlock &bb) const;
  const BasicBlock &idom(const BasicBlock &bb) const;

  // Return set of dominators of bb
  std::vector<BasicBlock *> dom(BasicBlock &bb) const;
  std::vector<const BasicBlock *> dom(const BasicBlock &bb) const;

  // List of successors in dominator tree
  std::vector<BasicBlock *> succs(BasicBlock &bb) const;
  std::vector<const BasicBlock *> succs(const BasicBlock &bb) const;

private:
  const Function &_fun;
  const CFG &_cfg;
  const VertexAdapter<const BasicBlock *> &_va;

  std::vector<std::size_t> _idom;
  std::vector<const BasicBlock *> _rpo;
  std::vector<std::size_t> _rpo_pos;
  Digraph _dtree;

  void _build();
  void _init();
  bool _iterate();
  std::size_t _intersect(std::size_t i, std::size_t j);
  void _build_dom_tree();
};

void idom_run(const Module &mod);
