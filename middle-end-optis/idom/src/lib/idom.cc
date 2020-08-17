#include "idom.hh"
#include "digraph.hh"

#include <fstream>
#include <iostream>

#include "cfg.hh"

namespace {

constexpr std::size_t UNDEF = -1;

class IDom {

public:
  IDom(const Function &fun) : _fun(fun), _cfg(_fun), _va(_cfg.va()) {}

  void run() {
    _init();

    std::size_t niter = 0;
    _dump(niter);

    bool changed = true;
    while (changed) {
      changed = _iterate();
      _dump(++niter);
    }
    _build_dom_tree();
  }

private:
  const Function &_fun;
  CFG _cfg;
  const VertexAdapter<const BasicBlock *> &_va;
  std::vector<std::size_t> _idom;
  std::vector<const BasicBlock *> _rpo;
  std::vector<std::size_t> _rpo_pos;

  // Build reverse postorder,
  // and init all idom to undef expect for first one
  void _init() {
    _rpo = _cfg.rev_postorder();
    assert(_rpo.front() == &_fun.get_entry_bb());
    _rpo_pos.resize(_rpo.size());
    for (std::size_t i = 0; i < _rpo.size(); ++i)
      _rpo_pos[_va(_rpo[i])] = i;

    _idom.assign(_va.size(), UNDEF);
    _idom[_va(&_fun.get_entry_bb())] = _va(&_fun.get_entry_bb());

    std::cout << "RPO: ";
    for (auto bb : _rpo)
      std::cout << bb->get_name() << ' ';
    std::cout << "\n";
    for (const auto &bb : _fun.bb()) {
      std::cout << bb.get_name() << ": " << _rpo_pos[_va(&bb)] << "; ";
    }
    std::cout << "\n\n";
  }

  // Run one iteration, and return true if any idom value changed
  bool _iterate() {
    bool changed = false;

    for (auto bb : _rpo) {
      if (bb == &_fun.get_entry_bb())
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
  std::size_t _intersect(std::size_t i, std::size_t j) {
    while (i != j) {
      while (_rpo_pos[i] > _rpo_pos[j])
        i = _idom[i];
      while (_rpo_pos[j] > _rpo_pos[i])
        j = _idom[j];
    }
    return i;
  }

  void _dump(std::size_t niter) {
    std::cout << "Iter #" << niter << ":\n";
    for (const auto &bb : _fun.bb()) {
      std::cout << bb.get_name() << ": ";
      auto n = _idom[_va(&bb)];
      std::cout << (n == UNDEF ? "X" : _va(n)->get_name());
      std::cout << "; ";
    }
    std::cout << "\n";
  }

  void _build_dom_tree() {
    Digraph g(_va.size());
    for (const auto &bb : _fun.bb()) {
      g.labels_set_vertex_name(_va(&bb), bb.get_name());
      if (&bb != &_fun.get_entry_bb())
        g.add_edge(_idom[_va(&bb)], _va(&bb));
    }

    std::ofstream ofs("dom_" + _fun.get_name() + ".dot");
    g.dump_tree(ofs);
  }
};

} // namespace

void idom_run(const Module &mod) {
  for (const auto &fun : mod.fun()) {
    if (!fun.has_def())
      continue;
    IDom dom(fun);
    dom.run();
  }
}
