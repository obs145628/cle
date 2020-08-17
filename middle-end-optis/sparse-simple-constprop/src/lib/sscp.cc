#include "sscp.hh"

#include <iostream>

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

std::ostream &operator<<(std::ostream &os, const Eval &v) {
  if (v.ty == EvalTy::TOP)
    os << "T";
  else if (v.ty == EvalTy::CONST)
    os << v.val;
  else if (v.ty == EvalTy::BOT)
    os << "B";
  return os;
}

class SSCP {

public:
  SSCP(Function &fun) : _fun(fun) {}

  void run() {
    // Init values
    _init();

    // Keep updating values until none change
    while (!_wlist.empty()) {
      auto &next = *_wlist.back();
      _wlist.pop_back();
      _propagate(next);
    }

    // Insert all const values in code
    for (auto it : _vals)
      if (it.second.ty == EvalTy::CONST)
        it.first->replace_all_uses_with(*ValueConst::make(it.second.val));

    std::cerr << "Final: \n";
    for (auto it : _vals)
      std::cerr << it.first->get_name() << ": " << it.second << "\n";
    std::cerr << "\n";
  }

private:
  Function &_fun;

  std::map<Instruction *, Eval> _vals;
  std::vector<Instruction *> _wlist;

  // Initialize all instructions by a quick analysis of the ops
  // Add all vals != T to the worklist
  void _init() {

    for (auto &bb : _fun.bb())
      for (auto &ins : bb.ins()) {
        if (!ins.has_def())
          continue;

        if (ins.get_opname() == "phi") {
          _vals[&ins] = EvalTy::TOP;
          continue;
        }

        if (ins.get_opname() == "mov") {
          auto src = dynamic_cast<ValueConst *>(&ins.op(0));
          if (src) {
            _vals[&ins] = src->get_val();
            continue;
          }
        }

        _vals[&ins] = EvalTy::TOP;
      }

    for (auto it : _vals)
      if (it.second.ty != EvalTy::TOP)
        _wlist.push_back(it.first);

    std::cerr << "Init: \n";
    for (auto it : _vals)
      std::cerr << it.first->get_name() << ": " << it.second << "\n";
  }

  void _propagate(Instruction &ins) {
    for (auto user : ins.get_users()) {
      auto user_ins = dynamic_cast<Instruction *>(user);
      if (!user_ins || !user_ins->has_def())
        continue;

      auto it = _vals.find(user_ins);
      assert(it != _vals.end());
      auto old_val = it->second;
      if (old_val.ty == EvalTy::BOT)
        continue;

      auto new_val = _eval(*user_ins);

      std::cerr << "Update " << user_ins->get_name() << ": " << old_val
                << " => " << new_val << "\n";

      it->second = new_val;
      if (old_val.ty != new_val.ty)
        _wlist.push_back(user_ins);
    }
  }

  Eval _eval_static(Value &val) {
    auto ins = dynamic_cast<Instruction *>(&val);
    assert(!ins);

    auto arg = dynamic_cast<ValueArg *>(&val);
    if (arg)
      return EvalTy::BOT;

    auto vconst = dynamic_cast<ValueConst *>(&val);
    if (vconst)
      return vconst->get_val();

    assert(0);
  }

  Eval _eval_lookup(Value &val) {
    auto it = _vals.find(dynamic_cast<Instruction *>(&val));
    if (it != _vals.end())
      return it->second;
    return _eval_static(val);
  }

  Eval _eval(Instruction &ins) {
    if (ins.get_opname() == "phi") {
      Eval res = EvalTy::TOP;
      for (std::size_t i = 1; i < ins.ops_count(); i += 2)
        res = Eval::meet(res, _eval_lookup(ins.op(i)));
      return res;
    }

    else if (ins.get_opname() == "mov") {
      return _eval_lookup(ins.op(1));
    }

    else if (ins.get_opname() == "add") {
      // B + x => B
      // 0 + x => x
      // T + x/T => T
      // x + y => x + y
      auto left = _eval_lookup(ins.op(0));
      auto right = _eval_lookup(ins.op(1));

      if (left.ty == EvalTy::BOT || right.ty == EvalTy::BOT)
        return EvalTy::BOT;

      if (left.is_const(0))
        return right;
      if (right.is_const(0))
        return left;

      if (left.ty == EvalTy::TOP || right.ty == EvalTy::TOP)
        return EvalTy::TOP;

      return left.val + right.val;
    }

    else {
      assert(0);
    }
  }
}; // namespace

} // namespace

void sscp_run(Module &mod) {
  for (auto &fun : mod.fun()) {
    if (!fun.has_def())
      continue;
    SSCP sscp(fun);
    sscp.run();
  }
}
