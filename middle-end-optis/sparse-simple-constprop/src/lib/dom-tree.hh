#pragma once

#include <vector>

#include "dom.hh"
#include "module.hh"
#include "vertex-adapter.hh"

// Build a dominance tree
// Compute the Immediate dominator of every bb
// u = IDom(b) iff:
//  - u strictly dominates b
//  - {v dominates u: v in Dom(b)}
// Every bb has one unique IDom, except the entry block which has none
// From the IDom, it's possible to get the Dom set of any block: recursively get
// the idoms until reaching the root
//
//
// Implementation build Dom sets firsts, find IDom for every node
class DomTree {
public:
  DomTree(const Function &fun);

  const BasicBlock &idom(const BasicBlock &bb) const;
  std::vector<const BasicBlock *> dom(const BasicBlock &bb) const;

  // Returns children of bb in Dom tree
  std::vector<const BasicBlock *> succs(const BasicBlock &bb) const;

private:
  const Function &_fun;
  const Dom _dom;
  VertexAdapter<const BasicBlock *> _va;
  std::vector<std::size_t> _preds;
  std::vector<std::size_t> _heights;

  void _build();

  void _find_idom(const BasicBlock *bb);
};
