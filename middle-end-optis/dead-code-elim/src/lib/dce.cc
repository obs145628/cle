#include "dce.hh"

#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <set>
#include <vector>

#include "cfg.hh"
#include "dom-frontier.hh"
#include "dom-tree.hh"

namespace {

class DCE {

public:
  DCE(llvm::Function &fun)
      : _fun(fun), _cfg(fun, /*reverse=*/true), _dtree(_fun, _cfg),
        _df(_fun, _dtree) {}

  void run() {
    _mark();
    _sweep();
  }

private:
  llvm::Function &_fun;
  CFG _cfg;
  DomTree _dtree;
  DomFrontier _df;

  std::set<llvm::Instruction *> _marked;
  std::set<llvm::BasicBlock *> _marked_bbs;

  // Mark all instructions used to make side effect on the program
  // (Not just the instructions with side effects, but also the instructions
  // that def values used by side-effect ins)
  void _mark() {
    // Mark all critical instructions
    std::vector<llvm::Instruction *> wlist;

    for (auto &bb : _fun)
      for (auto &ins : bb)
        if (_is_critical(ins)) {
          _marked.insert(&ins);
          wlist.push_back(&ins);
          llvm::errs() << "[C] Mark " << ins << "\n";
        }

    // Mark all instructions that def values used by critical instructions
    // And the conditional branch that may that lead to a block with a marked
    // ins
    while (!wlist.empty()) {
      auto &ins = *wlist.back();
      wlist.pop_back();

      for (auto &op : ins.operands()) {
        auto ins_op = llvm::dyn_cast<llvm::Instruction>(&op);
        if (!ins_op || _marked.count(ins_op))
          continue;

        _marked.insert(ins_op);
        wlist.push_back(ins_op);
        llvm::errs() << "[R] Mark " << *ins_op << "\n";
      }

      if (_marked_bbs.count(ins.getParent()))
        continue;

      // Mark a basicblock, and find all block in reverse dominance frontier
      _marked_bbs.insert(ins.getParent());

      for (auto b : _df.df(*ins.getParent())) {
        // Must be a conditional branch
        auto ins_br = llvm::dyn_cast<llvm::BranchInst>(b->getTerminator());
        assert(ins_br && ins_br->isConditional());
        if (!_marked.count(ins_br)) {
          _marked.insert(ins_br);
          wlist.push_back(ins_br);
          llvm::errs() << "[B] Mark " << *ins_br << "\n";
        }
      }
    }
  }

  // Remove all unmarked instructions
  void _sweep() {
    for (auto &bb : _fun) {
      auto it = bb.begin();
      while (it != bb.end()) {
        auto next = it;
        ++next;

        auto bins = llvm::dyn_cast<llvm::BranchInst>(&*it);
        bool is_jump = bins && bins->isUnconditional();

        if (!_marked.count(&*it) && !is_jump) {

          if (bins) {
            // Conditional jump, replace with jump to nearest marked
            // postdomiator
            auto target = &_dtree.idom(bb);
            while (!_marked_bbs.count(target)) {
              assert(target !=
                     &_dtree.root()); // root (exit BB) should always be marked
              target = &_dtree.idom(*target);
            }
            llvm::BranchInst::Create(target, bins);
          }

          // Cjump or not, erase instruction
          it->eraseFromParent();
        }

        it = next;
      }
    }
  }

  // Check if an instruction have side effets on the program and may get
  // executed
  // Consider only a subset of LLVM IR operations
  bool _is_critical(const llvm::Instruction &ins) {
    if (ins.getOpcode() == llvm::Instruction::Call)
      return true;
    if (ins.getOpcode() == llvm::Instruction::Ret)
      return true;

    return false;
  }
};

} // namespace

void dce_run(llvm::Module &mod) {
  for (auto &fun : mod) {
    if (fun.isDeclaration())
      continue;

    DCE dce(fun);
    dce.run();
  }
}
