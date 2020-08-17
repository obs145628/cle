#include "dvnt.hh"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "cfg.hh"
#include "idom.hh"

namespace {

using key_t = std::size_t;

constexpr key_t KEY_NONE = key_t(-1);

class ScopedTable {
  struct Level {
    std::vector<Value *> k2v;          // mapping key to value object
    std::map<Value *, key_t> v2k;      // mapping value object to key
    std::map<std::string, key_t> hmap; // hash map for expressions
  };

public:
  ScopedTable() = default;
  ~ScopedTable() { assert(_levels.empty()); }

  void open_scope() {
    if (_levels.empty())
      _levels.push_back(Level{});
    else
      _levels.push_back(_levels.back());
  }

  void close_scope() {
    assert(!_levels.empty());
    _levels.pop_back();
  }

  key_t get(Value &v) const {
    assert(!_levels.empty());
    const auto &l = _levels.back();
    auto it = l.v2k.find(&v);
    assert(it != l.v2k.end());
    return it->second;
  }

  Value &get(key_t k) const {
    assert(!_levels.empty());
    const auto &l = _levels.back();
    return *l.k2v.at(k);
  }

  key_t get(const std::string &h) const {
    assert(!_levels.empty());
    const auto &l = _levels.back();
    auto it = l.hmap.find(h);
    assert(it != l.hmap.end());
    return it->second;
  }

  key_t find(const std::string &h) const {
    assert(!_levels.empty());
    const auto &l = _levels.back();
    auto it = l.hmap.find(h);
    return it != l.hmap.end() ? it->second : KEY_NONE;
  }

  key_t add(Value &v) {
    assert(!_levels.empty());
    auto &l = _levels.back();
    key_t k = l.k2v.size();

    assert(l.v2k.emplace(&v, k).second);
    l.k2v.push_back(&v);
    return k;
  }

  void add_hash(const std::string &h, key_t k) {
    assert(!_levels.empty());
    auto &l = _levels.back();
    assert(k < l.k2v.size());
    assert(l.hmap.emplace(h, k).second);
  }

private:
  std::vector<Level> _levels;
};

class DVNT {

public:
  DVNT(Function &fun) : _fun(fun), _cfg(_fun), _idom(_fun, _cfg) {}

  void run() {
    // outer scope is for arguments and consts
    _table.open_scope();
    _simplify_consts();
    _add_args();

    // Simplify code by visiting nodes using dominance order
    _simplify(_fun.get_entry_bb());

    // Extra simplifications on the PHI node
    for (auto &bb : _fun.bb())
      _simplify_phis(bb);

    _table.close_scope();
  }

private:
  Function &_fun;
  CFG _cfg;
  IDom _idom;

  ScopedTable _table;

  void _add_args() {
    for (auto arg : _fun.args())
      _table.add(*arg);
  }

  // Combine ValueConsts with same value into one
  // And add them to the value list
  void _simplify_consts() {

    std::map<long, Value *> vmap;

    for (auto &bb : _fun.bb())
      for (auto &ins : bb.ins())
        for (auto v : ins.ops()) {
          auto vconst = dynamic_cast<ValueConst *>(v);
          if (!vconst)
            continue;
          auto val = vconst->get_val();

          if (vmap.count(val)) {
            v->replace_all_uses_with(*vmap[val]);
            std::cout << "Simplify const " << val << "\n";
          } else {
            vmap[val] = v;
            _table.add(*v);
          }
        }
  }

  void _simplify(BasicBlock &bb) {
    _table.open_scope();

    std::set<Instruction *> erased;

    for (auto &ins : bb.ins()) {
      if (!ins.has_def())
        continue;

      auto h = _get_hash(ins);
      auto key = _table.find(h);
      if (key == KEY_NONE) {
        key = _table.add(ins);
        if (!h.empty())
          _table.add_hash(h, key);
      } else {
        std::cout << "Found duplicate: ";
        ins.dump(std::cout);
        std::cout << "\n";
        ins.replace_all_uses_with(_table.get(key));
        erased.insert(&ins);
      }
    }

    for (auto ins : erased)
      ins->erase_from_parent();

    for (auto next : _idom.succs(bb))
      _simplify(*next);

    _table.close_scope();
  }

  void _simplify_phis(BasicBlock &bb) {

    std::set<Instruction *> erased;
    std::vector<std::vector<Value *>> prev_args;
    std::vector<Instruction *> prev_ins;

    for (auto &ins : bb.ins()) {
      if (ins.get_opname() != "phi")
        break;

      Value *unique_val = &ins.op(1);
      std::vector<Value *> args;
      for (std::size_t i = 1; i < ins.ops_count(); i += 2) {
        Value *val = &ins.op(i);
        args.push_back(val);
        if (val != unique_val)
          unique_val = nullptr;
      }

      // All phis operands have same value
      // Replace with this value
      // And delete phi node
      if (unique_val) {
        ins.replace_all_uses_with(*unique_val);
        erased.insert(&ins);
        std::cout << "Found useless phi: ";
        ins.dump(std::cout);
        std::cout << "\n";
      }

      // Check for duplicate phis
      std::size_t dup_id = 0;
      for (; dup_id < prev_args.size() && args != prev_args[dup_id]; ++dup_id)
        continue;

      if (dup_id == prev_args.size())
        prev_args.push_back(args);
      else {
        prev_args.push_back({});
        ins.replace_all_uses_with(*prev_ins[dup_id]);
        erased.insert(&ins);
        std::cout << "Found duplicate phi: ";
        ins.dump(std::cout);
        std::cout << "\n";
      }

      prev_ins.push_back(&ins);
    }

    for (auto ins : erased)
      ins->erase_from_parent();
  }

  std::string _get_hash(Instruction &ins) {
    if (ins.get_opname() == "phi") // phi's are handled later
      return "";
    if (ins.get_opname() == "call") // cant simplify, call may have side effects
      return "";

    std::string res = ins.get_opname();
    for (auto op : ins.ops())
      res += ":" + std::to_string(_table.get(*op));
    return res;
  }
};

} // namespace

void dvnt_run(Module &mod) {
  for (auto &fun : mod.fun()) {
    if (!fun.has_def())
      continue;
    DVNT dvnt(fun);
    dvnt.run();
  }
}
