#include "dom.hh"

#include <algorithm>
#include <iostream>

Dom::Dom(const Function &fun) : _fun(fun), _cfg(fun) { _run(); }

const Dom::dom_t &Dom::dom_of(const BasicBlock &bb) const {
  auto it = _doms.find(&bb);
  assert(it != _doms.end());
  return it->second;
}

void Dom::dump() const {
  for (const auto &bb : _fun.bb()) {
    std::cout << bb.label() << ": {";
    for (auto next : _doms.find(&bb)->second)
      std::cout << next->label() << " ";
    std::cout << "}\n";
  }
  std::cout << "\n";
}

void Dom::_run() {

  // 1) Init iteration order
  _compute_order();

  // 2) Intialize all BBs
  _init();

  // 3) Iterate over all doms until they doesn't change anymore
  for (;;) {
    bool udp = _update_all();
    if (!udp)
      break;
  }
}

// Initialize all dominance lists
// Dom(b_0) = {b_0}
// Dom(b) = N ; b != b_0
void Dom::_init() {
  auto entry = &_fun.get_entry_bb();
  dom_t all;
  for (const auto &bb : _fun.bb())
    all.insert(&bb);

  for (const auto &bb : _fun.bb())
    _doms.emplace(&bb, &bb == entry ? dom_t{entry} : all);
}

// Iterate over all basic blocks once to update it's DOM list
// return true if any of the list was updated
bool Dom::_update_all() {
  bool updated = false;
  for (auto bb : _order) {
    bool udp_bb = _update(*bb);
    updated |= udp_bb;
  }
  return updated;
}

// Update the DOM list of `bb` using the fixed-point equations
// Dom(n) = {n} | (&_{m in preds(n)} Dom(m))
// Returns true if list updated
bool Dom::_update(const BasicBlock &bb) {
  dom_t &old_dom = _doms.find(&bb)->second;
  dom_t next;
  bool first = true;

  // Compute new Dom
  for (auto pred : _cfg.preds(bb)) {
    const dom_t &pred_dom = _doms.find(pred)->second;
    if (first)
      next = pred_dom;
    else
      next = _inter(next, pred_dom);

    first = false;
  }
  next.insert(&bb);

  // Check if different from old one
  if (next.size() < old_dom.size()) {
    old_dom = next;
    return true;
  } else
    return false;
}

// Compute the order in which the basic block will be visited
// Choose order that reduce number of iterations
// Reverse Postorder make sure the preds of v are visited before v
// (topological sort)
// (property holds for acyclic graphs)
// Because formula use predecessors in computation, this reduces nomber of
// computations
// Remove entry block because it is never updated
void Dom::_compute_order() {
  _order = _cfg.rev_postorder();
  _order.erase(std::find(_order.begin(), _order.end(), &_fun.get_entry_bb()));

#if 0
  std::cout << "RPO: {";
  for (auto bb : _order)
    std::cout << bb->label() << " ";
  std::cout << "}\n\n";
#endif
}

// Set insersection
Dom::dom_t Dom::_inter(const dom_t &a, const dom_t &b) {
  dom_t res;
  for (auto x : a)
    if (b.find(x) != b.end())
      res.insert(x);
  return res;
}
