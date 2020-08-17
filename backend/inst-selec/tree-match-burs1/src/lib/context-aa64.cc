#include "context.hh"
#include "module-pass.hh"

#include <iostream>

#define ISA_IR (CMAKE_SRC_DIR "/config/lir.txt")
#define ISA_AA64 (CMAKE_SRC_DIR "/config/aa64.txt")
#define RULES_AA64 (CMAKE_SRC_DIR "/config/rules_aa64.txt")

namespace {

// Count and transform alloca instructions
class AllocaPass : public ModulePass {

public:
  AllocaPass() : _alloca_count(0) {}

protected:
  void _preprocess_fun(Function &fun) override {

    // First pass: count allocas
    for (const auto &bb : fun.bbs())
      for (const auto &ins : bb->code())
        if (ins[0] == "alloca")
          ++_alloca_count;

    // Second pass: replace allocas by offset from sp
    std::size_t off = 4 * _alloca_count + 8;
    for (auto &bb : fun.bbs())
      for (auto &ins : bb->code()) {
        if (ins[0] != "alloca")
          continue;
        off -= 4;
        ins = {"add", ins[1], "%sp", std::to_string(off)};
      }
  }

  void _update_asm_fun(Function &fun) override {
    if (_alloca_count == 0)
      return;

    std::size_t off = 4 * _alloca_count + 8;
    std::vector<std::string> beg_op{"sub", "%sp", "%sp", std::to_string(off)};
    std::vector<std::string> end_op{"add", "%sp", "%sp", std::to_string(off)};

    // Decrease sp at begining of entry block
    auto &entry = *fun.bbs()[0];
    entry.code().insert(entry.code().begin(), beg_op);

    for (auto &bb : fun.bbs()) {
      if (bb->code().back()[0] != "ret")
        continue;

      // Insert increase sp before every ret
      auto it = bb->code().end();
      --it;
      bb->code().insert(it, end_op);
    }
  }

private:
  std::size_t _alloca_count;
};

class BlockReorder : public ModulePass {

public:
protected:
  virtual void _update_asm_fun(Function &fun) override {
    _remove_useless_jumps(fun);
  }

private:
  void _remove_useless_jumps(Function &fun) {
    for (std::size_t i = 0; i + 1 < fun.bbs().size(); ++i) {
      auto &bb = *fun.bbs()[i];
      auto &next_bb = *fun.bbs()[i + 1];

      const auto &bins = bb.code().back();
      if (bins[0] == "b" && bins[1].substr(1) == next_bb.name()) {
        // jump to next ins, can be erased
        bb.code().pop_back();
      }
    }
  }
};

class ContextAa64 : public Context {

public:
  ContextAa64() {}
  ~ContextAa64() {}

protected:
  void _on_init() override {
    // Add custom passes
    add_module_pass<AllocaPass>();
    add_module_pass<BlockReorder>();

    // Add aa64 dependant infos to IR
    auto &ir = ir_ctx();
    ir.add_reg("sp");
  }

  std::string _get_isa_ir_file_path() override { return ISA_IR; }
  std::string _get_isa_asm_file_path() override { return ISA_AA64; }
  std::string _get_codegen_rules_file_path() override { return RULES_AA64; }

private:
};

REGISTER_CONTEXT("aa64", ContextAa64);

} // namespace
