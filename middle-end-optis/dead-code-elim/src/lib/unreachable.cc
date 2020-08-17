#include "unreachable.hh"

#include <iostream>
#include <set>

#include "cfg.hh"

namespace {

class Unreachable {

public:
  Unreachable(llvm::Function &fun) : _fun(fun), _cfg(_fun) {}

  void run() {
    // perform a DFS from the entry block to mark all reachable blocks
    _mark(_fun.getEntryBlock());

    // Remvoe all unmarked blocks
    _sweep();
  }

private:
  llvm::Function &_fun;
  const CFG _cfg;

  std::set<llvm::BasicBlock *> _reachable;
  int _count;

  void _mark(llvm::BasicBlock &bb) {
    _reachable.insert(&bb);
    for (auto succ : _cfg.succs(bb))
      if (!_reachable.count(succ))
        _mark(*succ);
  }

  void _sweep() {

    int count = 0;

    std::vector<llvm::BasicBlock *> bbs;
    for (auto &bb : _fun)
      bbs.push_back(&bb);

    for (auto bb : bbs)
      if (!_reachable.count(bb)) {
        std::cout << "Remove unreachable block " << std::string(bb->getName())
                  << "\n";
        bb->eraseFromParent();
        ++count;
      }

    std::cout << "Removed " << count << " unreachable basic blocks\n\n";
  }
};

} // namespace

void unreachable_run(llvm::Module &mod) {
  for (auto &fun : mod) {
    if (fun.isDeclaration())
      continue;

    Unreachable unr(fun);
    unr.run();
  }
}
