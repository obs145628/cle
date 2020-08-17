#pragma once

#include <map>
#include <set>

#include "cfg.hh"
#include "module.hh"

// Dominance
// Compute the dominance set of every basick block of a function
// block b_j in Dom(b_i) if all paths from b_0 to b_i contains b_j
// b_i in Dom(b_i)
// b_0 entry block
class Dom {
public:
  using dom_t = std::set<const BasicBlock *>;

  Dom(const Function &fun);

  const dom_t &dom_of(const BasicBlock &bb) const;

  void dump() const;

private:
  const Function &_fun;
  const CFG _cfg;
  std::vector<const BasicBlock *> _order;
  std::map<const BasicBlock *, dom_t> _doms;

  void _run();
  void _init();
  bool _update_all();
  bool _update(const BasicBlock &bb);
  void _compute_order();
  void _dump(int niter) const;
  static dom_t _inter(const dom_t &a, const dom_t &b);
};
