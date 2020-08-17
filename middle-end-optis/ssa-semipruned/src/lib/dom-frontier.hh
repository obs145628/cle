#pragma once

#include "cfg.hh"
#include "dom-tree.hh"
#include "module.hh"

// Compute the dominance frontier of every basic block in a function
// m in DF(n) iff:
// - n dominates a predecessor of m (q: q in preds(m) & n in Dom(q))
// - n does not strictly dominate m (n not in (Dom(m) - m))
//
// Informally, DF(n) contain the first node of every CFG path starting at n that
// n does not dominate.
class DomFrontier {

public:
  using df_t = std::set<const BasicBlock *>;

  DomFrontier(const Function &fun);

  const df_t &df(const BasicBlock &bb) const;

  const DomTree &dom_tree() const { return _dt; }

  void dump() const;

private:
  const Function &_fun;
  const CFG _cfg;
  const DomTree _dt;
  std::map<const BasicBlock *, df_t> _dfs;

  void _build();
};
