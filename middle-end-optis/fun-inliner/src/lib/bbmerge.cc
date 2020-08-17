#include "bbmerge.hh"
#include "cfg.hh"

#include <cassert>
#include <map>
#include <set>

namespace {

class BBMerge {
public:
  BBMerge(Function &fun) : _fun(fun), _cfg(fun) {}

  void run() {

    auto &entry = _fun.get_entry_bb();
    _work.insert(&entry);
    while (!_work.empty()) {
      auto bb = *_work.begin();
      _work.erase(_work.begin());
      _run(*bb);
    }

    // @extra all basick blocks not in done are unreachable
    // they can be deleted from the function
  }

private:
  Function &_fun;
  CFG _cfg;
  std::set<BasicBlock *> _done;
  std::set<BasicBlock *> _work; // worklist

  // `bb` can be merged with another bb if this bb has only one successor which
  // has only 1 pred: bb
  void _run(BasicBlock &bb) {
    assert(_done.count(&bb) == 0);
    _done.insert(&bb);

    auto succs = _cfg.succs(bb);

    // there may be a chain of many BBs to merge
    while (succs.size() == 1 && _cfg.preds(*succs[0]).size() == 1) {
      // can merge bb withs succs[0]
      auto succ = succs[0];
      succs = _cfg.succs(*succ);

      // remove jump
      bb.ins().back().erase_from_parent();
      // move all instruction at the end of bb
      BasicBlock::ins_move(*succ, succ->ins_begin(), succ->ins_end(), bb,
                           bb.ins_end());
      // remove empty succ
      succ->erase_from_parent();
    }

    for (auto succ : succs)
      if (_done.count(succ) == 0)
        _work.insert(succ);
  }
};

} // namespace

void bbmerge(Function &fun) {
  BBMerge bbm(fun);
  bbm.run();
}
