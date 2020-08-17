#include "dom.hh"

#include <llvm/Support/raw_ostream.h>
#include <set>
#include <vector>

#include "cfg.hh"

namespace {

class Dom {

  using dom_t = std::set<const llvm::BasicBlock *>;

public:
  Dom(const llvm::Function &fun) : _fun(fun), _cfg(fun) {}

  void run() {

    // 1) Init iteration order
    _compute_order();

    // 2) Intialize all BBs
    _init();

    int niter = 0;
    _dump(niter);

    // 3) Iterate over all doms until they doesn't change anymore
    for (;;) {
      bool udp = _update_all();
      _dump(++niter);
      if (!udp)
        break;
    }
  }

private:
  const llvm::Function &_fun;
  const CFG _cfg;
  std::vector<const llvm::BasicBlock *> _order;
  std::map<const llvm::BasicBlock *, dom_t> _doms;

  // Initialize all dominance lists
  // Dom(b_0) = {b_0}
  // Dom(b) = N ; b != b_0
  void _init() {
    auto entry = &_fun.getEntryBlock();
    dom_t all;
    for (const auto &bb : _fun)
      all.insert(&bb);

    for (const auto &bb : _fun)
      _doms.emplace(&bb, &bb == entry ? dom_t{entry} : all);
  }

  // Iterate over all basic blocks once to update it's DOM list
  // return true if any of the list was updated
  bool _update_all() {
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
  bool _update(const llvm::BasicBlock &bb) {
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
  void _compute_order() {
    _order = _cfg.rev_postorder();
    _order.erase(
        std::find(_order.begin(), _order.end(), &_fun.getEntryBlock()));

    llvm::errs() << "RPO: {";
    for (auto bb : _order)
      llvm::errs() << bb->getName() << " ";
    llvm::errs() << "}\n\n";
  }

  void _dump(int niter) const {
    llvm::errs() << "Iter #" << niter << ":\n";
    for (const auto &bb : _fun) {
      llvm::errs() << bb.getName() << ": {";
      for (auto next : _doms.find(&bb)->second)
        llvm::errs() << next->getName() << " ";
      llvm::errs() << "}\n";
    }
    llvm::errs() << "\n";
  }

  // Set insersection
  static dom_t _inter(const dom_t &a, const dom_t &b) {
    dom_t res;
    for (auto x : a)
      if (b.find(x) != b.end())
        res.insert(x);
    return res;
  }
};

} // namespace

void dom_run(const llvm::Module &mod) {
  for (const auto &fun : mod) {
    Dom dom(fun);
    dom.run();
  }
}
