#include "dom-frontier.hh"

#include <cassert>
#include <iostream>

DomFrontier::DomFrontier(llvm::Function &fun, const DomTree &dt)
    : _fun(fun), _dt(dt), _cfg(_dt.cfg()) {
  _build();
  dump();
}

const DomFrontier::df_t &DomFrontier::df(llvm::BasicBlock &bb) const {
  auto it = _dts.find(&bb);
  assert(it != _dts.end());
  return it->second;
}

void DomFrontier::dump() const {
  for (auto &bb : _fun) {
    const auto &set = df(bb);
    std::cout << std::string(bb.getName()) << ": DF: {";
    for (auto x : set)
      std::cout << std::string(x->getName()) << " ";
    std::cout << "}, ";

    std::cout << "Dom: {";
    for (auto x : _dt.dom(bb))
      std::cout << std::string(x->getName()) << " ";
    std::cout << "}, ";

    std::cout << "Idom: ";
    if (&bb == &_dt.root())
      std::cout << "--";
    else
      std::cout << std::string(_dt.idom(bb).getName());
    std::cout << "\n";
  }
  std::cout << "\n";
}

void DomFrontier::_build() {

  // Init all sets to empty
  for (auto &bb : _fun)
    _dts.emplace(&bb, df_t{});

  for (auto &bb : _fun) {
    auto preds = _cfg.preds(bb);
    if (preds.size() < 2) // bb can't be in fronter is has only one pred
      continue;

    for (auto p : preds) {
      llvm::BasicBlock *prev = p;
      while (prev != &_dt.idom(bb)) {
        // Add all items in dom(p) that are not in dom(bb)
        // Stop at soon as it finds a block in dom(bb)
        //   (This block always is Idom(bb))
        _dts[prev].insert(&bb);
        prev = &_dt.idom(*prev);
      }
    }
  }
}
