#include "ssa.hh"

#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include "cfg.hh"
#include "dom-frontier.hh"
#include "isa.hh"

namespace {

template <class K, class V> class ScopedMap {

public:
  using iterator_t = typename std::map<K, V>::const_iterator;

  ScopedMap() : _stack({std::map<K, V>{}}) {}

  ~ScopedMap() { assert(_stack.size() == 1); }

  void open() { _stack.push_back(std::map<K, V>{}); }

  void close() {
    assert(_stack.size() > 1);
    _stack.pop_back();
  }

  iterator_t put(const K &key, const V &val) {
    assert(_stack.size() > 1);
    auto ret = _stack.back().emplace(key, val);
    if (!ret.second)
      ret.first->second = val;
    return ret.first;
  }

  iterator_t find(const K &key) const {
    assert(_stack.size() > 1);
    for (auto sit = _stack.rbegin(); sit != _stack.rend(); ++sit) {
      auto it = sit->find(key);
      if (it != sit->end())
        return it;
    }

    return end();
  }

  iterator_t end() const { return _stack[0].end(); }

private:
  std::vector<std::map<K, V>> _stack;
};

class SSA {
public:
  SSA(Function &fun) : _fun(fun), _cfg(_fun), _df(fun) {}

  void run() {
    // Find globals and defs
    _prepare();

    // Insert phis
    _find_phis();
    _prune_phis();
    _insert_phis();

    // Rename all registers
    _rename();

    _dump();
  }

private:
  Function &_fun;
  CFG _cfg;
  DomFrontier _df;

  // use of reg defined in another bb
  std::set<std::string> _globals;

  // [def d] => set of bbs where d is defined
  std::map<std::string, std::set<const BasicBlock *>> _blocks;

  // bb => {set of defs that need phy at start of bb}
  std::map<const BasicBlock *, std::set<std::string>> _phis;

  std::map<std::string, std::size_t> _next_ids;

  // Init _globals and _blocks
  void _prepare() {
    for (const auto &bb : _fun.bb()) {
      std::set<std::string> varkill;

      for (const auto &ins : bb.ins()) {
        for (const auto &r : isa::uses(ins))
          if (!varkill.count(r))
            _globals.insert(r);

        for (const auto &r : isa::defs(ins)) {
          varkill.insert(r);
          _blocks[r].insert(&bb);
        }
      }
    }
  }

  // Find where to insert phis in the function
  void _find_phis() {
    for (const auto &bb : _fun.bb())
      _phis.emplace(&bb, std::set<std::string>{});

    for (const auto &def : _globals) {
      auto wlist = _blocks.find(def)->second;

      while (!wlist.empty()) {
        auto bb = *wlist.begin();
        wlist.erase(wlist.begin());

        for (auto d : _df.df(*bb))
          if (_phis[d].insert(def).second)
            wlist.insert(d);
      }
    }
  }

  // Prune phis that are invalid
  // There are phis that refer to variable not defined in the dom path
  // leading to it
  void _prune_phis() {
    ScopedMap<std::string, int> defs;
    const auto &bb = _fun.get_entry_bb();
    _prune_phis_rec(bb, defs);
  }

  void _prune_phis_rec(const BasicBlock &bb,
                       ScopedMap<std::string, int> &defs) {
    defs.open();

    // Insert all defs in scoped map
    for (const auto &ins : bb.ins())
      for (const auto &r : isa::defs(ins))
        defs.put(r, 1);

    // Remove all phis of CFG succs not in defs
    for (auto next : _cfg.succs(bb)) {
      auto &phis = _phis.find(next)->second;
      std::vector<std::string> invalid;
      for (const auto &r : phis) {
        if (defs.find(r) == defs.end())
          invalid.push_back(r);
      }
      for (const auto &r : invalid)
        phis.erase(r);
    }

    // Visit successors in Dom Tree
    for (auto next : _df.dom_tree().succs(bb))
      _prune_phis_rec(*next, defs);

    defs.close();
  }

  // Insert phis not renamed yet where needed
  void _insert_phis() const {
    for (auto &bb : _fun.bb()) {
      const auto &phis = _phis.find(&bb)->second;
      if (phis.empty())
        continue;

      auto first = bb.ins().begin();
      for (const auto &def : phis) {
        std::vector<std::string> phi_ins{"phi", def};
        for (auto pred : _cfg.preds(bb)) {
          phi_ins.push_back("@" + pred->label());
          phi_ins.push_back(def);
        }
        bb.insert_ins(first, phi_ins);
      }
    }
  }

  std::string _rename_def(const std::string &name) {
    std::size_t id = 0;
    auto it = _next_ids.find(name);
    if (it != _next_ids.end())
      id = it->second;
    _next_ids[name] = id + 1;

    return name + std::to_string(id);
  }

  void _rename() {
    ScopedMap<std::string, std::string> new_names;
    _rename_bb(_fun.get_entry_bb(), new_names);
  }

  void _rename_bb(BasicBlock &bb,
                  ScopedMap<std::string, std::string> &new_names) {
    new_names.open();

    for (auto &ins : bb.ins()) {

      // Rename all uses
      if (ins.args[0] != "phi")
        for (auto &r : isa::uses(ins)) {
          auto it = new_names.find(r);
          assert(it != new_names.end());
          r = it->second;
        }

      // Generate new names for all defs
      for (auto &r : isa::defs(ins)) {
        auto new_def = _rename_def(r);
        new_names.put(r, new_def);
        r = new_def;
      }
    }

    // Rename phi arguments of successors in CFG
    for (auto next : _cfg.succs(bb)) {
      auto &next_bb = *_fun.get_bb(next->label());
      for (auto &ins : next_bb.ins()) {
        if (ins.args[0] != "phi")
          break;

        std::size_t bb_idx = 0;
        while (ins.args[bb_idx] != "@" + bb.label())
          ++bb_idx;

        auto &r = ins.args[bb_idx + 1];
        auto it = new_names.find(r);
        assert(it != new_names.end());
        r = it->second;
      }
    }

    // Visit successors in Dom Tree
    for (auto next : _df.dom_tree().succs(bb)) {
      auto next_bb = _fun.get_bb(next->label());
      _rename_bb(*next_bb, new_names);
    }

    new_names.close();
  }

  void _dump() const {
    _df.dump();

    std::cout << "globals: {";
    for (const auto &r : _globals)
      std::cout << r << ", ";
    std::cout << "}\n";

    std::cout << "blocks:\n";
    for (const auto &block : _blocks) {
      std::cout << block.first << ": {";
      for (auto bb : block.second)
        std::cout << bb->label() << ", ";
      std::cout << "}\n";
    }
    std::cout << "\n";

    std::cout << "phis:\n";
    for (const auto &phis : _phis) {
      if (phis.second.empty())
        continue;
      std::cout << phis.first->label() << ": {";
      for (const auto &def : phis.second)
        std::cout << def << ", ";
      std::cout << "}\n";
    }
    std::cout << "\n";
  }
};

} // namespace

void ssa_run(Module &mod) {
  for (auto &fun : mod.fun()) {
    SSA ssa(fun);
    ssa.run();
  }
}
