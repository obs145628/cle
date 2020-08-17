#include "dcfe.hh"

#include <fstream>
#include <iostream>
#include <llvm/IR/Instructions.h>
#include <set>

#include "cfg.hh"

namespace {

// Only contains a terminator instruction
bool is_empty(const llvm::BasicBlock &bb) { return bb.front().isTerminator(); }

bool term_is_cbr(const llvm::BasicBlock &bb) {
  auto bins = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
  return bins && bins->isConditional();
}

bool term_is_jump(const llvm::BasicBlock &bb) {
  auto bins = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
  return bins && bins->isUnconditional();
}

llvm::BranchInst *clone_branch(const llvm::BranchInst *ins,
                               llvm::BasicBlock *insert_point) {
  if (ins->isConditional())
    return llvm::BranchInst::Create(ins->getSuccessor(0), ins->getSuccessor(1),
                                    ins->getCondition(), insert_point);
  else
    return llvm::BranchInst::Create(ins->getSuccessor(0), insert_point);
}

class DCFE {
public:
  DCFE(llvm::Function &fun) : _fun(fun), _cfg(_fun) {}

  // Iteratively run transformations until the code doesn't change anymore
  void run() {
    int npass = 1;
    bool changed = true;
    while (changed) {
      std::cout << "DCFE Pass #" << npass++ << ":\n";
      changed = _run_pass();
    }
    std::cout << "\n";

    std::ofstream ofs("cfg_" + std::string(_fun.getName()) + ".dot");
    _cfg.graph().dump_tree(ofs);
  }

private:
  llvm::Function &_fun;
  CFG _cfg;

  // skip basic blocks removed from code, but still in CFG
  // (vertices cannot be removed from digraph)
  std::set<llvm::BasicBlock *> _removed;

  // Identify and run as many of the 4 transformations as possible
  // Use postorder to run successors first (when no cycles), to reduce number of
  // passes Return if code changed
  bool _run_pass() {
    bool changed = false;

    for (auto bb : _cfg.postorder())
      for (;;) {
        if (_removed.count(bb)) // A prev or current pass might remove block
          break;
        // Run multiple passes in same BB until it doesn't change
        bool bb_changed = _run_pass(*bb);
        if (!bb_changed)
          break;
        changed = true;
      }

    return changed;
  }

  bool _run_pass(llvm::BasicBlock &bb) {
    if (term_is_cbr(bb)) {
      auto bins = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
      if (bins->getSuccessor(0) == bins->getSuccessor(1)) // Case 1
        return _tr_redundant(bb);
    }

    if (!term_is_jump(bb))
      return false;

    auto bins = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
    auto next = bins->getSuccessor(0);

    if (is_empty(bb)) // Case 2
      return _tr_empty(bb);

    if (_cfg.preds(*next).size() == 1) // Case 3
      return _tr_combine(bb);

    if (is_empty(*next) && term_is_cbr(*next)) // Case 4
      return _tr_hoist(bb);

    return false;
  }

  // Case 1: Fold a Redundant Branch
  // Replace a cbr to 2 identicals target by a jump
  bool _tr_redundant(llvm::BasicBlock &bb) {
    auto bins = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
    assert(bins && bins->getNumSuccessors() == 2);
    auto t1 = bins->getSuccessor(0);
    auto t2 = bins->getSuccessor(1);
    assert(t1 == t2);
    llvm::BranchInst::Create(t1, bins);
    bins->eraseFromParent();
    // CFG not changed
    std::cout << "  Fold Redundant(" << std::string(bb.getName()) << ")\n";
    return true;
  }

  // Case 2: Remove an empty block
  // Bb contains only a jump
  // Remove bb, and replace all branches to bb by branch to it's successor
  bool _tr_empty(llvm::BasicBlock &bb) {
    auto bins = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
    assert(bins && bins->getNumSuccessors() == 1);
    assert(&bb.front() == bins);
    auto next = bins->getSuccessor(0);
    assert(&bb != next);

    bb.replaceAllUsesWith(next);
    bb.eraseFromParent();

    // Update CFG
    auto &g = _cfg.graph();
    const auto &va = _cfg.va();
    g.del_edge(va(&bb), va(next));
    for (auto p : _cfg.preds(bb)) {
      g.del_edge(va(p), va(&bb));
      g.add_edge(va(p), va(next));
    }

    _removed.insert(&bb);
    std::cout << "  Remove Empty(" << std::string(bb.getName()) << ")\n";
    return true;
  }

  // Case 3: Combine 2 blocks
  // when i ends in jump to j, and j has only one predecessor i
  // Combine block i and j, and remove block j
  bool _tr_combine(llvm::BasicBlock &bb) {
    auto bins = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
    assert(bins && bins->getNumSuccessors() == 1);
    auto next = bins->getSuccessor(0);
    assert(&bb != next);
    assert(_cfg.preds(*next).size() == 1);
    std::cout << "  Combine(" << std::string(bb.getName()) << ", "
              << std::string(next->getName()) << ")\n";

    // Delete jump in i and move all instructions from j to i
    // then delete j
    bb.back().eraseFromParent();
    while (!next->empty())
      next->front().moveAfter(&bb.back());
    next->eraseFromParent();

    // Update CFG
    auto &g = _cfg.graph();
    const auto &va = _cfg.va();
    g.del_edge(va(&bb), va(next));
    for (auto s : g.succs(va(next))) {
      g.del_edge(va(next), s);
      g.add_edge(va(&bb), s);
    }

    _removed.insert(next);
    return true;
  }

  // Case 4: Hoist a Branch
  // i jump to j, which is an empty block that ends with a branch
  // j cannot be deleted because it has multiple preds
  // the jump in i is replaced by the branch in j
  bool _tr_hoist(llvm::BasicBlock &bb) {
    auto bins = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
    assert(bins && bins->getNumSuccessors() == 1);
    auto next = bins->getSuccessor(0);
    assert(&bb != next);
    assert(is_empty(*next));
    auto next_br = llvm::dyn_cast<llvm::BranchInst>(next->getTerminator());
    assert(next_br && next_br->getNumSuccessors() == 2);
    assert(_cfg.preds(*next).size() > 1);
    std::cout << "  Hoist(" << std::string(bb.getName()) << ", "
              << std::string(next->getName()) << ")\n";

    bins->eraseFromParent();
    clone_branch(next_br, &bb);

    // Update CFG
    auto &g = _cfg.graph();
    const auto &va = _cfg.va();
    g.del_edge(va(&bb), va(next));
    for (auto s : g.succs(va(next)))
      g.add_edge(va(&bb), s);

    return true;
  }
};

} // namespace

void dcfe_run(llvm::Module &mod) {

  for (auto &fun : mod) {
    if (fun.isDeclaration())
      continue;

    DCFE dcfe(fun);
    dcfe.run();
  }
}
