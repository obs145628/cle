#include "idom.hh"

#include <fstream>

#include "cfg.hh"

namespace {

constexpr std::size_t UNDEF = -1;

}

IDom::IDom(const Function &fun, const CFG &cfg)
    : _fun(fun), _cfg(cfg), _va(_cfg.va()), _dtree(_va.size()) {
  _build();
}

const BasicBlock &IDom::root() const { return _fun.get_entry_bb(); }

BasicBlock &IDom::idom(BasicBlock &bb) const {
  assert(&bb != &root());
  const auto &mva = _cfg.mva(bb);
  return *mva(_idom.at(mva(&bb)));
}

const BasicBlock &IDom::idom(const BasicBlock &bb) const {
  assert(&bb != &root());
  return *_va(_idom.at(_va(&bb)));
}

std::vector<BasicBlock *> IDom::dom(BasicBlock &bb) const {
  std::vector<BasicBlock *> res;

  BasicBlock *node = &bb;
  while (node != &root()) {
    res.push_back(node);
    node = &idom(*node);
  }

  res.push_back(node);
  return res;
}

std::vector<const BasicBlock *> IDom::dom(const BasicBlock &bb) const {
  std::vector<const BasicBlock *> res;

  const BasicBlock *node = &bb;
  while (node != &root()) {
    res.push_back(node);
    node = &idom(*node);
  }

  res.push_back(node);
  return res;
}

std::vector<BasicBlock *> IDom::succs(BasicBlock &bb) const {
  std::vector<BasicBlock *> res;
  auto &mva = _cfg.mva(bb);

  for (auto u : _dtree.succs(_va(&bb)))
    res.push_back(mva(u));
  return res;
}

std::vector<const BasicBlock *> IDom::succs(const BasicBlock &bb) const {
  std::vector<const BasicBlock *> res;

  for (auto u : _dtree.succs(_va(&bb)))
    res.push_back(_va(u));
  return res;
}

void IDom::_build() {
  _init();

  while (_iterate())
    continue;

  _build_dom_tree();
}

// Build reverse postorder,
// and init all idom to undef expect for first one
void IDom::_init() {
  _rpo = _cfg.rev_postorder();
  assert(_rpo.front() == &root());
  _rpo_pos.resize(_rpo.size());
  for (std::size_t i = 0; i < _rpo.size(); ++i)
    _rpo_pos[_va(_rpo[i])] = i;

  _idom.assign(_va.size(), UNDEF);
  _idom[_va(&root())] = _va(&root());
}

// Run one iteration, and return true if any idom value changed
bool IDom::_iterate() {
  bool changed = false;

  for (auto bb : _rpo) {
    if (bb == &root())
      continue;

    auto new_idom = UNDEF;
    for (auto pred : _cfg.preds(*bb)) {
      if (_idom[_va(pred)] == UNDEF)
        continue;
      if (new_idom == UNDEF)
        new_idom = _va(pred);
      else
        new_idom = _intersect(_va(pred), new_idom);
    }
    assert(new_idom != UNDEF);

    if (_idom[_va(bb)] != new_idom) {
      _idom[_va(bb)] = new_idom;
      changed = true;
    }
  }

  return changed;
}

// Compute the intersection of the 2 doms sets, correspondig to node i and j,
// using only the idom
// This correspond the closest node in the dom-tree that is a predecessor of
// both i and j
std::size_t IDom::_intersect(std::size_t i, std::size_t j) {
  while (i != j) {
    while (_rpo_pos[i] > _rpo_pos[j])
      i = _idom[i];
    while (_rpo_pos[j] > _rpo_pos[i])
      j = _idom[j];
  }
  return i;
}

void IDom::_build_dom_tree() {
  for (const auto &bb : _fun.bb()) {
    _dtree.labels_set_vertex_name(_va(&bb), bb.get_name());
    if (&bb != &root())
      _dtree.add_edge(_idom[_va(&bb)], _va(&bb));
  }

  std::ofstream ofs("dom_" + _fun.get_name() + ".dot");
  _dtree.dump_tree(ofs);
}
