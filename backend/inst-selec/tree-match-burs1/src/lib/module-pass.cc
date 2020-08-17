#include "module-pass.hh"

void ModulePass::preprocess(Module &mod) { _preprocess_mod(mod); }

void ModulePass::update_asm(Module &mod) { _update_asm_mod(mod); }

void ModulePass::_preprocess_mod(Module &mod) {
  for (auto fun : mod.funs())
    _preprocess_fun(*fun);
}

void ModulePass::_preprocess_fun(Function &fun) {
  for (auto bb : fun.bbs())
    _preprocess_bb(*bb);
}

void ModulePass::_preprocess_bb(BasicBlock &) {}

void ModulePass::_update_asm_mod(Module &mod) {
  for (auto fun : mod.funs())
    _update_asm_fun(*fun);
}

void ModulePass::_update_asm_fun(Function &fun) {
  for (auto bb : fun.bbs())
    _update_asm_bb(*bb);
}

void ModulePass::_update_asm_bb(BasicBlock &) {}
