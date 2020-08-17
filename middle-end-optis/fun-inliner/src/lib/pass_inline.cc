#include "pass_inline.hh"

#include <cassert>
#include <iostream>

#include "bbmerge.hh"
#include "inline.hh"

namespace {

// Return the first function call instruction that can be inline, or nullptr if
// none
Instruction *find_inlinable(Function &fun) {
  const auto &mod = fun.parent();

  for (auto &bb : fun.bb())
    for (auto &ins : bb.ins()) {
      if (ins.args[0] == "call") {
        auto name = ins.args[1][0] == '%' ? ins.args[2] : ins.args[1];
        assert(name[0] == '@');
        auto callee = mod.get_fun(name.substr(1));
        assert(callee);
        if (callee != &fun) // doesn't inline recursive calls
          return &ins;
      }
    }

  return nullptr;
}

void run_on_fun(Function &fun) {

  bool changed = false;
  (void)changed;

  for (;;) {
    Instruction *call_site = find_inlinable(fun);
    if (!call_site)
      break;
    std::cout << "Inlining " << *call_site << " (" << fun.name() << ")\n";
    inline_call(*call_site);
    changed = true;
  }

  if (changed)
    bbmerge(fun);
}

} // namespace

void run_inline_pass(Module &mod) {
  for (auto &fun : mod.fun())
    run_on_fun(fun);
  mod.check();
}
