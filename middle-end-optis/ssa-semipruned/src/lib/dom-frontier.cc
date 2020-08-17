#include "dom-frontier.hh"

#include <cassert>
#include <iostream>

DomFrontier::DomFrontier(const Function &fun)
    : _fun(fun), _cfg(_fun), _dt(_fun) {
  _build();
}

const DomFrontier::df_t &DomFrontier::df(const BasicBlock &bb) const {
  auto it = _dfs.find(&bb);
  assert(it != _dfs.end());
  return it->second;
}

void DomFrontier::dump() const {
  for (const auto &bb : _fun.bb()) {
    const auto &set = df(bb);
    std::cout << bb.label() << ": DF: {";
    for (auto x : set)
      std::cout << x->label() << " ";
    std::cout << "}, ";

    std::cout << "Dom: {";
    for (auto x : _dt.dom(bb))
      std::cout << x->label() << " ";
    std::cout << "}, ";

    std::cout << "Idom: ";
    if (&bb == &_fun.get_entry_bb())
      std::cout << "--";
    else
      std::cout << _dt.idom(bb).label();
    std::cout << "\n";
  }
  std::cout << "\n";
}

void DomFrontier::_build() {

  // Init all sets to empty
  for (const auto &bb : _fun.bb())
    _dfs.emplace(&bb, df_t{});

  for (const auto &bb : _fun.bb()) {
    auto preds = _cfg.preds(bb);
    if (preds.size() < 2) // bb can't be in fronter is has only one pred
      continue;

    for (auto p : preds) {
      const BasicBlock *prev = p;
      while (prev != &_dt.idom(bb)) {
        // Add all items in dom(p) that are not in dom(bb)
        // Stop at soon as it finds a block in dom(bb)
        //   (This block always is Idom(bb))
        _dfs[prev].insert(&bb);
        prev = &_dt.idom(*prev);
      }
    }
  }
}
