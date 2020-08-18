#include "dom-tree.hh"

#include <fstream>
#include <iostream>

#include "digraph.hh"
#include "dom.hh"

namespace {

constexpr std::size_t NODE_UNDEF = -1;

}

DomTree::DomTree(const Function &fun)
    : _fun(fun), _dom(fun),
      _va(fun.bb().map([](const BasicBlock &bb) { return &bb; })) {
  _build();
}

const BasicBlock &DomTree::idom(const BasicBlock &bb) const {
  auto v = _va(&bb);
  assert(_heights[v] != 0);
  return *_va(_preds[v]);
}

std::vector<const BasicBlock *> DomTree::dom(const BasicBlock &bb) const {
  const BasicBlock *node = &bb;
  std::vector<const BasicBlock *> res{node};

  while (_heights[_va(node)] != 0) {
    node = &idom(*node);
    res.push_back(node);
  }

  return res;
}

std::vector<const BasicBlock *> DomTree::succs(const BasicBlock &bb) const {
  std::vector<const BasicBlock *> res;
  for (const auto &child : _fun.bb()) {
    if (&_fun.get_entry_bb() == &child)
      continue;

    if (&idom(child) == &bb)
      res.push_back(&child);
  }

  return res;
}

void DomTree::_build() {
  _preds.assign(_va.size(), NODE_UNDEF);
  _heights.assign(_va.size(), NODE_UNDEF);

  for (std::size_t i = 0; i < _va.size(); ++i)
    _find_idom(_va(i));

  // Check if idom of all nodes computed
  for (std::size_t i = 0; i < _va.size(); ++i)
    assert(_heights[i] != NODE_UNDEF);

  // Build and dump digraph dot file
  Digraph dt(_va.size());
  for (const auto &bb : _fun.bb()) {
    dt.labels_set_vertex_name(_va(&bb), bb.label());
    if (&_fun.get_entry_bb() != &bb)
      dt.add_edge(_preds[_va(&bb)], _va(&bb));
  }
  std::ofstream ofs("idom_" + _fun.name() + ".dot");
  dt.dump_tree(ofs);
}

void DomTree::_find_idom(const BasicBlock *bb) {
  // already computed
  if (_heights[_va(bb)] != NODE_UNDEF)
    return;

  // entry is root
  if (bb == &_fun.get_entry_bb()) {
    _preds[_va(bb)] = NODE_UNDEF;
    _heights[_va(bb)] = 0;
    return;
  }

  // compute idom of all blocks in (Dom(bb) - bb)
  // Idom(bb) is the one with the largest height
  const BasicBlock *idom = nullptr;
  for (auto pred : _dom.dom_of(*bb)) {
    if (pred == bb)
      continue;

    _find_idom(pred);
    if (!idom || _heights[_va(pred)] > _heights[_va(idom)])
      idom = pred;
  }

  // Add bb to tree
  assert(idom);
  _preds[_va(bb)] = _va(idom);
  _heights[_va(bb)] = _heights[_va(idom)] + 1;
}
