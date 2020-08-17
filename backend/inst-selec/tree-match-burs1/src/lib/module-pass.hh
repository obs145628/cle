#pragma once

#include "module.hh"

class ModulePass {

public:
  ModulePass() = default;
  virtual ~ModulePass() = default;

  // Preprocess the IR Module
  void preprocess(Module &mod);

  // Run on ASM module after tree matching
  void update_asm(Module &mod);

private:
  virtual void _preprocess_mod(Module &mod);
  virtual void _preprocess_fun(Function &fun);
  virtual void _preprocess_bb(BasicBlock &bb);

  virtual void _update_asm_mod(Module &mod);
  virtual void _update_asm_fun(Function &fun);
  virtual void _update_asm_bb(BasicBlock &bb);
};
