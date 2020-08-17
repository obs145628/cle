#include "scc.hh"

#include <cassert>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

#include "cfg.hh"

namespace {

enum class EvalTy {
  TOP,
  CONST,
  BOT,
};

struct Eval {
  EvalTy ty;
  int val;

  Eval() : Eval(0) {}

  Eval(EvalTy ty) : ty(ty), val(0) {}

  Eval(int val) : ty(EvalTy::CONST), val(val) {}

  bool is_const() const { return ty == EvalTy::CONST; }

  bool is_const(int c) const { return ty == EvalTy::CONST && val == c; }

  static Eval meet(const Eval &x, const Eval &y) {
    if (x.ty == EvalTy::BOT || y.ty == EvalTy::BOT)
      return EvalTy::BOT;
    if (x.ty == EvalTy::TOP)
      return y;
    if (y.ty == EvalTy::TOP)
      return x;

    if (x.val == y.val)
      return x;
    else
      return EvalTy::BOT;
  }
};

bool operator==(const Eval &x, const Eval &y) {
  return x.ty == y.ty && x.val == y.val;
}

bool operator!=(const Eval &x, const Eval &y) { return !(x == y); }

std::ostream &operator<<(std::ostream &os, const Eval &v) {
  if (v.ty == EvalTy::TOP)
    os << "T";
  else if (v.ty == EvalTy::CONST)
    os << v.val;
  else if (v.ty == EvalTy::BOT)
    os << "B";
  return os;
}

class SCC {

  using cfg_edge_t = std::pair<BasicBlock *, BasicBlock *>;
  using ssa_edge_t = std::pair<Value *, Value *>;

public:
  SCC(Function &fun) : _fun(fun), _cfg(_fun) {}

  void run() {
    _init_vals();
    _init_executed();

    _cfg_wl.push_back(cfg_edge_t(nullptr, &_fun.get_entry_bb()));
    _iterate();

    std::cout << "\n";
    _dump_executed();
    _dump_vals();

    _update_code();
  }

private:
  Function &_fun;
  CFG _cfg;

  std::vector<cfg_edge_t> _cfg_wl;
  std::vector<ssa_edge_t> _ssa_wl;
  std::map<Value *, Eval> _vals;
  std::vector<int> _executed_map;

  // Iterate until both worklists are empty
  void _iterate() {
    while (!_cfg_wl.empty() || !_ssa_wl.empty()) {
      if (!_cfg_wl.empty()) {
        auto e = _cfg_wl.front();
        _cfg_wl.erase(_cfg_wl.begin());
        _iterate(e);
      }

      if (!_ssa_wl.empty()) {
        auto e = _ssa_wl.front();
        _ssa_wl.erase(_ssa_wl.begin());
        _iterate(e);
      }
    }
  }

  void _iterate(cfg_edge_t e) {
    if (e.first != nullptr && _is_executed(e))
      return;

    auto &bb = *e.second;
    _set_executed(e);
    _eval_phis(bb);

    // Another edge (x, bb) != e already executed
    // No need to exec again
    if (_get_exec_preds(&bb).size() > 1)
      return;

    // In the original paper, a block cannot have more than one expression
    // A bb with many ins is the same as each ins in its own block, followed by
    // branch to next
    // I think the result is the same if I go through all the
    // instructions in order
    // The risk is I must go through them in order, and
    // never eval an instruction in the middle of the block before a preceding
    // one in this block
    // (otherwhise I may get TOP vals while evaluating operands, this would not
    // happen in original paper because instruction wouldn't be evalued is there
    // is no executed incomming edge)
    for (auto &ins : bb.ins())
      if (ins.get_opname() != "phi")
        _eval_ins(ins);
  }

  void _iterate(ssa_edge_t e) {
    auto d = dynamic_cast<Instruction *>(e.first);
    auto u = dynamic_cast<Instruction *>(e.second);
    assert(d && d->has_def());
    assert(u);

    // Skip if the block s isn't executed
    // (is there any edge x -> c executed)
    // Otherwhise use may be dead
    if (_get_exec_preds(&u->parent()).empty())
      return;

    if (u->get_opname() == "phi")
      _eval_phi(*u);
    else
      _eval_ins(*u);
  }

  void _init_vals() {
    // Set val of all instructions, even those without def
    // If no def, value has another usage (eg for beq it's the final condition
    // value)

    for (auto &bb : _fun.bb()) {
      for (auto &ins : bb.ins()) {
        _vals[&ins] = EvalTy::TOP;
      }
    }
  }

  void _dump_vals() {
    std::cout << "Values: {\n";
    for (auto it : _vals) {
      std::cout << "  ";
      it.first->dump(std::cout);
      std::cout << ": " << it.second << "\n";
    }
    std::cout << "}\n\n";
  }

  // Compute the value of an instruction given its operands
  // All operands must have been evalued already (value != TOP)
  // It has a custom implementation for every operands
  void _eval_ins(Instruction &ins) {
    assert(ins.get_opname() != "phi");

    auto old_val = _vals.at(&ins);
    if (old_val == EvalTy::BOT)
      return;

    // Compute new value
    Eval new_val;

    if (ins.get_opname() == "add") {
      auto left = _get_op_val(ins.op(0));
      auto right = _get_op_val(ins.op(1));
      if (left.is_const() && right.is_const())
        new_val = left.val + right.val;
      else
        new_val = EvalTy::BOT;
    }

    else if (ins.get_opname() == "sub") {
      auto left = _get_op_val(ins.op(0));
      auto right = _get_op_val(ins.op(1));
      if (left.is_const() && right.is_const())
        new_val = left.val - right.val;
      else
        new_val = EvalTy::BOT;
    }

    else if (ins.get_opname() == "mul") {
      auto left = _get_op_val(ins.op(0));
      auto right = _get_op_val(ins.op(1));
      if (left.is_const() && right.is_const())
        new_val = left.val * right.val;
      else if (left == 0 || right == 0)
        new_val = 0;
      else
        new_val = EvalTy::BOT;
    }

    else if (ins.get_opname() == "ret")
      new_val = _get_op_val(ins.op(0));

    else if (ins.get_opname() == "b") {
      // In the paper, this is handled at the end of the part that handles a new
      // CFG node Handling here doesn't change anything It's still get called
      // only once (b has no uses)
      auto target = dynamic_cast<BasicBlock *>(&ins.op(0));
      assert(target);
      _cfg_wl.push_back(cfg_edge_t(&ins.parent(), target));
    }

    else if (ins.get_opname() == "b") {
      // In the paper, this is handled at the end of the part that handles a new
      // CFG node Handling here doesn't change anything It's still get called
      // only once (b has no uses)
      auto target = dynamic_cast<BasicBlock *>(&ins.op(0));
      assert(target);
      _cfg_wl.push_back(cfg_edge_t(&ins.parent(), target));
    }

    else if (ins.get_opname() == "beq") {
      auto left = _get_op_val(ins.op(0));
      auto right = _get_op_val(ins.op(1));
      if (left.is_const() && right.is_const())
        new_val = left.val == right.val;
      else
        new_val = EvalTy::BOT;

      auto target_true = dynamic_cast<BasicBlock *>(&ins.op(2));
      auto target_false = dynamic_cast<BasicBlock *>(&ins.op(3));
      assert(target_true);
      assert(target_false);

      if (new_val.is_const()) {
        auto final_target = new_val == 1 ? target_true : target_false;
        std::cout << "Revolsed cjump to " << final_target->get_name() << "\n";
        _cfg_wl.push_back(cfg_edge_t(&ins.parent(), final_target));
      } else {
        _cfg_wl.push_back(cfg_edge_t(&ins.parent(), target_true));
        _cfg_wl.push_back(cfg_edge_t(&ins.parent(), target_false));
      }
    }

    if (old_val == new_val)
      return;

    std::cout << "Ins update: ";
    ins.dump(std::cout);
    std::cout << old_val << " -> " << new_val << "\n";

    _vals[&ins] = new_val;
    for (auto u : ins.get_users()) // Only assignment have users
      _ssa_wl.push_back(ssa_edge_t(&ins, u));
  }

  // Evaluate all phis at the entry of bb
  void _eval_phis(BasicBlock &bb) {
    for (auto &ins : bb.ins()) {
      if (ins.get_opname() != "phi")
        break;
      _eval_phi(ins);
    }
  }

  void _eval_phi(Instruction &ins) {
    assert(ins.get_opname() == "phi");
    auto old_val = _vals.at(&ins);
    if (old_val == EvalTy::BOT)
      return;

    auto n = &ins.parent();

    Eval new_val = EvalTy::TOP;
    for (std::size_t i = 0; i < ins.ops_count(); i += 2) {
      auto m = dynamic_cast<BasicBlock *>(&ins.op(i));
      assert(m);
      if (!_is_executed(cfg_edge_t(m, n)))
        continue;
      // Meet only with executed branches
      new_val = Eval::meet(new_val, _get_op_val(ins.op(i + 1)));
    }

    assert(new_val != EvalTy::TOP); // at least one edge is executed
    if (old_val == new_val)
      return;

    std::cout << "Phi update: ";
    ins.dump(std::cout);
    std::cout << old_val << " -> " << new_val << "\n";

    _vals[&ins] = new_val;
    for (auto u : ins.get_users())
      _ssa_wl.push_back(ssa_edge_t(&ins, u));
  }

  // For most value it simply replace all uses
  // Some instructions are special cases (doesn't produce a val, but must change
  // something on the code)
  void _update_code() {

    for (auto it : _vals) {
      if (!it.second.is_const())
        continue;

      auto ins = dynamic_cast<Instruction *>(it.first);

      if (ins && ins->get_opname() == "ret") {
        ins->set_op(0, *ValueConst::make(it.second.val));
      }

      else if (ins && ins->get_opname() == "beq") {
        // Nothing to do
        // Another pass will replace it with an unconditional jump
      }

      else if (ins && ins->get_opname() == "b") {
        // Nothing to do here
      }

      // Common case
      else {
        assert(ins && ins->has_def());
        ins->replace_all_uses_with(*ValueConst::make(it.second.val));
      }
    }
  }

  Eval _get_op_val(Value &val) {
    // We should never try to eval a bb / fun, makes no sense
    assert(!dynamic_cast<BasicBlock *>(&val));
    assert(!dynamic_cast<Function *>(&val));

    if (auto vconst = dynamic_cast<ValueConst *>(&val))
      return vconst->get_val();

    if (dynamic_cast<ValueArg *>(&val))
      return EvalTy::BOT; // arguments are unknown

    auto ins = dynamic_cast<Instruction *>(&val);
    assert(ins && ins->has_def());

    auto res = _vals.at(&val);
    assert(res !=
           EvalTy::TOP); // because of SSA definition and we only go through
                         // executable code, the value cannot be top
    return res;
  }

  void _dump_executed() {
    std::cout << "Executed: {\n";
    for (auto &b1 : _fun.bb())
      for (auto b2 : _cfg.succs(b1)) {
        auto e = cfg_edge_t(&b1, b2);
        bool exec = _is_executed(e);
        std::cout << "  " << b1.get_name() << " -> " << b2->get_name() << ": "
                  << (exec ? "true" : "false") << "\n";
      }
    std::cout << "}\n\n";
  }

  void _init_executed() {
    _executed_map.assign(_cfg.graph().v() * _cfg.graph().v(), 0);
  }

  std::vector<BasicBlock *> _get_exec_preds(BasicBlock *bb) {
    assert(bb);
    if (bb == &_fun.get_entry_bb())
      return {nullptr};

    std::vector<BasicBlock *> res;
    for (auto p : _cfg.preds(*bb))
      if (_is_executed(cfg_edge_t(p, bb)))
        res.push_back(p);
    return res;
  }

  bool _is_executed(const cfg_edge_t &e) {
    if (!e.first) // Entry block => always executed
      return true;

    return _executed_map[_cfg.va()(e.first) * _cfg.graph().v() +
                         _cfg.va()(e.second)];
  }

  void _set_executed(const cfg_edge_t &e) {
    if (!e.first) // Entry block => already set executed
      return;
    assert(!_is_executed(e));
    _executed_map[_cfg.va()(e.first) * _cfg.graph().v() + _cfg.va()(e.second)] =
        1;
  }
};

} // namespace

void scc_run(Module &mod) {
  for (auto &fun : mod.fun()) {
    if (!fun.has_def())
      continue;
    SCC sbc(fun);
    sbc.run();
  }
}
