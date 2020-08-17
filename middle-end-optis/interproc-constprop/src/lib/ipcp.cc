#include "ipcp.hh"

#include <iostream>
#include <memory>
#include <set>
#include <vector>

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

  operator int() const {
    assert(ty == EvalTy::CONST);
    return val;
  }

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

struct CallSite;
struct ProcInfos;

using support_t = std::set<ValueArg *>;

struct CallSite {

  ProcInfos &proc;
  Instruction &ins;
  std::vector<support_t> supports;

  CallSite(ProcInfos &proc, Instruction &ins)
      : proc(proc), ins(ins), supports(ins.ops_count() - 1) {}
};

struct ProcInfos {

  Function &fun;
  std::vector<CallSite *> calls;
  std::vector<Eval> args;

  ProcInfos(Function &fun) : fun(fun) {}
};

class IPCP {

public:
  IPCP(Module &mod) : _mod(mod) {}

  void run() {

    // Gather all staic informations about functions and call sites needed
    // during the algorithm
    _prepare();

    // Set all arguments val to an intial start value and add all to the work
    // list
    _init();
    _dump("init");

    // Iteratively update arguments value until fixed point reached
    while (!_wlist.empty()) {
      auto arg = _wlist.back();
      _wlist.pop_back();
      _propagate(arg);
    }
    _dump("\nfinal");

    // Update code
    for (auto &proc : _procs)
      for (std::size_t i = 0; i < proc->args.size(); ++i)
        if (proc->args[i].ty == EvalTy::CONST)
          proc->fun.get_arg(i).replace_all_uses_with(
              *ValueConst::make(proc->args[i]));
  }

private:
  Module &_mod;
  std::vector<std::unique_ptr<ProcInfos>> _procs;
  std::vector<std::unique_ptr<CallSite>> _sites;
  std::map<Function *, ProcInfos *> _procs_map;
  std::map<ValueArg *, ProcInfos *> _args_map;
  std::vector<ValueArg *> _wlist;

  void _prepare() {
    // List all defined functions
    for (auto &fun : _mod.fun()) {
      if (!fun.has_def())
        continue;

      _procs.push_back(std::make_unique<ProcInfos>(fun));
      auto proc = _procs.back().get();
      proc->args.assign(fun.args_count(), EvalTy::TOP);
      _procs_map.emplace(&fun, proc);
      for (auto arg : fun.args())
        _args_map.emplace(arg, proc);
    }

    // List all call sites to defined functions
    for (auto &fun : _mod.fun()) {
      if (!fun.has_def())
        continue;
      auto proc = _procs_map.at(&fun);

      for (auto &bb : fun.bb())
        for (auto &ins : bb.ins()) {
          if (ins.get_opname() != "call")
            continue;

          auto &callee = dynamic_cast<Function &>(ins.op(0));
          auto it = _procs_map.find(&callee);
          if (it == _procs_map.end()) // call to function without definition
            continue;

          auto &callee_proc = *it->second;
          _sites.push_back(std::make_unique<CallSite>(callee_proc, ins));
          auto site = _sites.back().get();
          proc->calls.push_back(site);
          for (std::size_t i = 0; i < callee_proc.fun.args_count(); ++i)
            _build_support(*site, i);
        }
    }

    std::cout << "Found " << _procs.size() << " functions and " << _sites.size()
              << " call sites\n";
  }

  void _init() {
    for (auto &proc : _procs)
      for (std::size_t i = 0; i < proc->fun.args_count(); ++i) {
        proc->args[i] = EvalTy::TOP;
        _wlist.push_back(&proc->fun.get_arg(i));
      }

    for (auto &site : _sites) {
      auto &proc = site->proc;
      for (std::size_t i = 0; i < proc.args.size(); ++i)
        proc.args[i] =
            Eval::meet(proc.args[i], _eval_static(site->ins.op(i + 1)));
    }
  }

  void _propagate(ValueArg *arg) {
    auto &proc = *_args_map.at(arg);

    for (auto site : proc.calls) {
      auto &callee = site->proc;
      for (std::size_t i = 0; i < site->supports.size(); ++i) {
        if (!site->supports[i].count(arg))
          continue;
        auto old_val = callee.args[i];
        auto new_val = Eval::meet(old_val, _eval_static(site->ins.op(i + 1)));
        if (old_val.ty != new_val.ty) {
          callee.args[i] = new_val;
          _wlist.push_back(&callee.fun.get_arg(i));

          std::cout << "Update " << callee.fun.get_name() << "#" << i << ": "
                    << old_val << " => " << new_val << "\n";
        }
      }
    }
  }

  // Build support set for a parameter value
  // Trivial support: non-empty only if arg is directly a function parameter
  void _build_support(CallSite &site, std::size_t arg) {
    auto &val = site.ins.op(arg + 1);
    auto arg_val = dynamic_cast<ValueArg *>(&val);
    if (arg_val)
      site.supports[arg].insert(arg_val);
  }

  Eval _eval_static(Value &val) {
    auto vconst = dynamic_cast<ValueConst *>(&val);
    if (vconst)
      return vconst->get_val();

    auto arg = dynamic_cast<ValueArg *>(&val);
    if (!arg)
      return EvalTy::BOT;

    auto &proc = *_args_map.at(arg);
    std::size_t i;
    for (i = 0; i < proc.args.size(); ++i)
      if (&proc.fun.get_arg(i) == arg)
        break;
    assert(i != proc.args.size());
    return proc.args[i];
  }

  void _dump(const std::string &msg) {
    std::cout << msg << ":\n";
    for (auto &proc : _procs)
      for (std::size_t i = 0; i < proc->args.size(); ++i)
        std::cout << proc->fun.get_name() << "#" << i << ": " << proc->args[i]
                  << "\n";
    std::cout << "\n";
  }
};

} // namespace

void ipcp_run(Module &mod) {
  IPCP ipcp(mod);
  ipcp.run();
}
