#include "context.hh"

#define ISA_IR (CMAKE_SRC_DIR "/config/lir.txt")
#define ISA_X64 (CMAKE_SRC_DIR "/config/x64.txt")
#define RULES_X64 (CMAKE_SRC_DIR "/config/rules_x64.txt")

namespace {

// Count and transform alloca instructions
class AllocaPass : public ModulePass {

public:
  AllocaPass() : _alloca_count(0) {}

protected:
  void _preprocess_fun(Function &fun) override {
    long off = 0;
    // count and replace allocas
    for (auto &bb : fun.bbs())
      for (auto &ins : bb->code()) {
        if (ins[0] != "alloca")
          continue;
        ++_alloca_count;
        off -= 4;
        ins = {"add", ins[1], "%rbp", std::to_string(off)};
      }
  }

  void _update_asm_fun(Function &fun) override {
    if (_alloca_count == 0)
      return;

    std::vector<std::vector<std::string>> beg_op{{"pushq", "%rbp"},
                                                 {"movq", "%rsp", "%rbp"}};
    std::vector<std::vector<std::string>> end_op{{"popq", "%rbp"}};

    // Save and update stack pointer in prologue
    auto &entry = *fun.bbs()[0];
    entry.code().insert(entry.code().begin(), beg_op.begin(), beg_op.end());

    for (auto &bb : fun.bbs()) {
      if (bb->code().back()[0] != "retq")
        continue;

      // Restore stack pointer before every ret
      auto it = bb->code().end();
      --it;
      bb->code().insert(it, end_op.begin(), end_op.end());
    }
  }

private:
  std::size_t _alloca_count;
};

class ContextX64 : public Context {

public:
  ContextX64() {}
  ~ContextX64() {}

protected:
  void _on_init() override {
    // Add custom passes
    add_module_pass<AllocaPass>();

    // Add x64 dependant infos to IR
    auto &ir = ir_ctx();
    ir.add_reg("rbp");
    ir.add_reg("rsp");
  }

  std::string _get_isa_ir_file_path() override { return ISA_IR; }
  std::string _get_isa_asm_file_path() override { return ISA_X64; }
  std::string _get_codegen_rules_file_path() override { return RULES_X64; }

private:
};

REGISTER_CONTEXT("x64", ContextX64);

} // namespace
