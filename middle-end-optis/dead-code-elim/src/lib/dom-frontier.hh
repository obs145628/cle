#pragma once

#include <set>

#include "cfg.hh"
#include "dom-tree.hh"

// Compute the dominance frontier of every basic block in a function
// m in DF(n) iff:
// - n dominates a predecessor of m (q: q in preds(m) & n in Dom(q))
// - n does not strictly dominate m (n not in (Dom(m) - m))
//
// Informally, DF(n) contain the first node of every CFG path starting at n that
// n does not dominate.
class DomFrontier {

public:
  using df_t = std::set<llvm::BasicBlock *>;

  DomFrontier(llvm::Function &fun, const DomTree &dt);

  const df_t &df(llvm::BasicBlock &bb) const;

  void dump() const;

private:
  llvm::Function &_fun;
  const DomTree &_dt;
  const CFG &_cfg;
  std::map<llvm::BasicBlock *, df_t> _dts;

  void _build();
};
