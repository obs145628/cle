#include "sbc.hh"

#include <iostream>

#include "cfg.hh"
#include "cloner.hh"
#include "digraph-order.hh"

namespace {

constexpr std::size_t PRED_NONE = -1;

class CyclesFinder {

public:
  CyclesFinder(const Digraph &g) : _g(g) { _run(); }

  bool has_cycle() const { return !_cycle.empty(); }

  const std::vector<std::size_t> &get_cycle() const {
    assert(has_cycle());
    return _cycle;
  }

private:
  const Digraph &_g;

  std::vector<std::size_t> _marked;
  std::vector<std::size_t> _preds;
  std::vector<std::size_t> _on_stack;

  std::vector<std::size_t> _cycle;

  void _run() {
    _marked.assign(_g.v(), 0);
    _preds.assign(_g.v(), PRED_NONE);
    _on_stack.assign(_g.v(), 0);

    for (std::size_t i = 0; i < _g.v(); ++i)
      if (!_marked[i])
        _dfs(i);
  }

  void _dfs(std::size_t u) {
    _marked[u] = 1;
    _on_stack[u] = 1;

    for (auto w : _g.succs(u)) {
      if (has_cycle())
        return;

      _preds[w] = u;

      if (_on_stack[w]) {
        // Cycle detected
        _build_cycle(w);
        return;
      }

      else if (!_marked[w]) {
        _dfs(w);
      }
    }

    _on_stack[u] = 0;
  }

  void _build_cycle(std::size_t w) {

    std::vector<std::size_t> rcycle;
    std::size_t node = w;

    while (_preds[node] != w) {
      assert(_preds[node] != PRED_NONE);
      node = _preds[node];
      rcycle.push_back(node);
    }

    rcycle.push_back(w);

    _cycle.assign(rcycle.rbegin(), rcycle.rend());
  }
};

class SBC {
public:
  SBC(Function &fun) : _fun(fun), _cfg(_fun) {}

  void run() {
    _head = &_find_loop();
    std::cout << "Loop head: " << _head->get_name() << "\n";
    _build_backward();

    _clone_rec(*_head);

    // Need to fix phis afterward (otherwhise we may fix a block already cloned)
    for (auto bb : _to_fix)
      _fix_phis(*bb.second, *bb.first);

    std::cout << "\n";

    CFG new_cfg(_fun);
  }

private:
  Function &_fun;
  CFG _cfg;
  Cloner _cloner;

  BasicBlock *_head;
  // Used to detect backward branches
  std::map<const BasicBlock *, std::size_t> _dfs_order;

  std::map<const BasicBlock *, std::vector<BasicBlock *>> _clones;
  std::map<BasicBlock *, BasicBlock *> _original;
  std::map<BasicBlock *, BasicBlock *> _to_fix; // need to fix phis on these
                                                // ones

  // Find the loop head
  BasicBlock &_find_loop() {
    CyclesFinder cf(_cfg.graph());
    return *_cfg.mva(_fun)(cf.get_cycle().front());
  }

  void _build_backward() {
    auto dfs = digraph_dfs(_cfg.graph(), DFSOrder::REV_POST);

    std::cout << "DFS order (RevPost): ";
    for (std::size_t i = 0; i < dfs.size(); ++i) {
      _dfs_order[_cfg.va()(dfs[i])] = i;
      std::cout << _cfg.va()(dfs[i])->get_name() << " ";
    }
    std::cout << "\n";

    for (auto &bb : _fun.bb())
      for (auto succ : _cfg.succs(bb))
        if (_is_backward(bb, *succ))
          std::cout << "Edge " << bb.get_name() << " -> " << succ->get_name()
                    << " is backward.\n";
  }

  bool _is_backward(const BasicBlock &b1, const BasicBlock &b2) {
    return _dfs_order.at(&b2) < _dfs_order.at(&b1);
  }

  void _clone_rec(BasicBlock &bb) {
    // successors should be the original ones
    auto succs = _cfg.succs(_get_original(bb));
    for (auto s : succs)
      assert(&_get_original(*s) == s);

    // stop if node has any backward branch
    for (auto s : succs)
      if (_is_backward(_get_original(bb), *s))
        return;

    if (succs.size() != 1) {
      for (auto s : succs)
        _clone_rec(*s);
      return;
    }

    // Only one branch: clone it and jump to it
    auto &bins = bb.ins().back();
    assert(bins.get_opname() == "b");
    BasicBlock &next_bb = _do_clone(*succs[0]);
    bins.set_op(0, next_bb);
    _to_fix.emplace(&bb, &next_bb);

    _clone_rec(next_bb);
  }

  BasicBlock &_do_clone(BasicBlock &bb) {
    assert(&_get_original(bb) == &bb);
    auto &clones = _clones[&bb];

    auto base_name = bb.get_name();
    if (!clones.empty())
      base_name = base_name.substr(0, base_name.size() - 2);
    std::cout << "Cloning block " << base_name << "\n";

    // Only need to clone if they are 2 versions
    if (clones.empty()) {
      bb.set_name(base_name + "_0");
      clones.push_back(&bb);
      return bb;
    }

    BasicBlock &new_bb = _fun.add_bb();
    new_bb.set_name(base_name + "_" + std::to_string(clones.size()));
    _cloner.clone_bb(bb, new_bb, new_bb.ins_begin());
    clones.push_back(&new_bb);
    _original.emplace(&new_bb, &bb);
    return new_bb;
  }

  // After cloning, bb has only one predecessor left, and doesn't need phi
  // blocks anymore
  void _fix_phis(BasicBlock &bb, BasicBlock &pred) {
    auto &tpred = _get_original(pred);
    std::vector<Instruction *> deleted;

    for (auto &ins : bb.ins()) {
      if (ins.get_opname() != "phi")
        break;

      Value *new_val = nullptr;
      for (std::size_t i = 0; i < ins.ops_count(); i += 2) {
        auto phi_pred = dynamic_cast<BasicBlock *>(&ins.op(i));
        assert(phi_pred);
        if (phi_pred == &tpred)
          new_val = &ins.op(i + 1);
      }

      assert(new_val);
      ins.replace_all_uses_with(*new_val);
      deleted.push_back(&ins);
    }

    for (auto ins : deleted)
      ins->erase_from_parent();
  }

  // find original from cloned block
  // Return itself if already an original
  BasicBlock &_get_original(BasicBlock &bb) {
    auto it = _original.find(&bb);
    if (it == _original.end())
      return bb;
    else
      return *it->second;
  }
};

} // namespace

void sbc_run(Module &mod) {
  for (auto &fun : mod.fun()) {
    if (!fun.has_def())
      continue;
    SBC sbc(fun);
    sbc.run();
  }
}
