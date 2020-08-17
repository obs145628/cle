#include "dom-tree.hh"

#include <fstream>
#include <iostream>

namespace {

constexpr std::size_t UNDEF = -1;

}

DomTree::DomTree(llvm::Function &fun, const CFG &cfg)
    : _fun(fun), _cfg(cfg), _va(_cfg.va()) {
  _run();
}

llvm::BasicBlock &DomTree::idom(llvm::BasicBlock &bb) const {
  assert(&bb != _root);
  return *_va(_idom[_va(&bb)]);
}

std::vector<llvm::BasicBlock *> DomTree::dom(llvm::BasicBlock &bb) const {
  std::vector<llvm::BasicBlock *> res;
  llvm::BasicBlock *node = &bb;

  while (node != _root) {
    res.push_back(node);
    node = &idom(*node);
  }

  res.push_back(node);
  return res;
}

void DomTree::_run() {
  _init();

  std::size_t niter = 0;
  _dump(niter);

  bool changed = true;
  while (changed) {
    changed = _iterate();
    _dump(++niter);
  }
  _build_dom_tree();
  std::cout << "\n";
}

// Build reverse postorder,
// and init all idom to undef expect for first one
void DomTree::_init() {
  _root = &_find_root();
  _rpo = _cfg.rev_postorder();
  assert(_rpo.front() == _root);
  _rpo_pos.resize(_rpo.size());
  for (std::size_t i = 0; i < _rpo.size(); ++i)
    _rpo_pos[_va(_rpo[i])] = i;

  _idom.assign(_va.size(), UNDEF);
  _idom[_va(_root)] = _va(_root);

  std::cout << "RPO: ";
  for (auto bb : _rpo)
    std::cout << std::string(bb->getName()) << ' ';
  std::cout << "\n";
  for (auto &bb : _fun) {
    std::cout << std::string(bb.getName()) << ": " << _rpo_pos[_va(&bb)]
              << "; ";
  }
  std::cout << "\n\n";
}

// Run one iteration, and return true if any idom value changed
bool DomTree::_iterate() {
  bool changed = false;

  for (auto bb : _rpo) {
    if (bb == _root)
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
std::size_t DomTree::_intersect(std::size_t i, std::size_t j) {
  while (i != j) {
    while (_rpo_pos[i] > _rpo_pos[j])
      i = _idom[i];
    while (_rpo_pos[j] > _rpo_pos[i])
      j = _idom[j];
  }
  return i;
}

void DomTree::_dump(std::size_t niter) {
  std::cout << "Iter #" << niter << ":\n";
  for (auto &bb : _fun) {
    std::cout << std::string(bb.getName()) << ": ";
    auto n = _idom[_va(&bb)];
    std::cout << (n == UNDEF || &bb == _root ? std::string("X")
                                             : std::string(_va(n)->getName()));
    std::cout << "; ";
  }
  std::cout << "\n";
}

void DomTree::_build_dom_tree() {
  Digraph g(_va.size());
  for (auto &bb : _fun) {
    g.labels_set_vertex_name(_va(&bb), bb.getName());
    if (&bb != _root)
      g.add_edge(_idom[_va(&bb)], _va(&bb));
  }

  std::ofstream ofs("dom_" + std::string(_fun.getName()) + ".dot");
  g.dump_tree(ofs);
}

llvm::BasicBlock &DomTree::_find_root() {
  auto &entry = _fun.getEntryBlock();
  if (_cfg.preds(entry).empty())
    return entry;

  // Reverse CFG
  for (auto &bb : _fun) {
    auto bins = bb.getTerminator();
    if (bins->getOpcode() == llvm::Instruction::Ret)
      return bb;
  }

  assert(0);
}
