#include <cstring>
#include <fstream>
#include <iostream>

#include "isa/isa.hh"
#include "lib/matcher.hh"
#include "lib/rules.hh"
#include "lib/tree-block.hh"
#include <gop10/module.hh>

#define ISA_IR (CMAKE_SRC_DIR "/config/lir.txt")
#define ISA_AA64 (CMAKE_SRC_DIR "/config/aa64.txt")
#define ISA_X64 (CMAKE_SRC_DIR "/config/x64.txt")

#define RULES_AA64 (CMAKE_SRC_DIR "/config/rules_aa64.txt")
#define RULES_X64 (CMAKE_SRC_DIR "/config/rules_x64.txt")

#define ISA_ASM (is_x64 ? ISA_X64 : ISA_AA64)
#define RULES (is_x64 ? RULES_X64 : RULES_AA64)

// Tree-Matching implementation
// My first try of implementation
// Only works on a syngle basic block, which must be a-tree like cgraph
// (not a digraph)
// Really basic version
//
// Inspired from:
// Instruction Selection via Tree-Pattern Matching - Enginner a Compiler p610

std::unique_ptr<gop::Module> init_mod(const std::string &fun_name) {
  auto mod = std::make_unique<gop::Module>();
  auto fun = std::make_unique<gop::Dir>(std::vector<std::string>{"fun"});
  fun->label_defs.push_back(fun_name);
  mod->decls.push_back(std::move(fun));
  return mod;
}

int main(int argc, char **argv) {

  bool is_x64;

  if (argc < 3) {
    std::cerr << "Usage: ./iselec-tree-match-naive <arch> <ir-file>"
              << std::endl;
    return 1;
  }

  if (!std::strcmp(argv[1], "x64"))
    is_x64 = true;
  else if (!std::strcmp(argv[1], "aa64"))
    is_x64 = false;
  else {
    std::cerr << "Unknown arch " << argv[1] << "\n";
    return 1;
  }

  std::ifstream ifs(argv[2]);
  auto mod = gop::Module::parse(ifs);

  isa::Context ir_ctx(ISA_IR);
  isa::check(ir_ctx, mod);

  mod.dump(std::cout);
  std::cout << "\n";

  TreeBlock tb(ir_ctx, mod);
  Rules rules(RULES);
  Matcher matcher(rules, tb);
  matcher.run();

  isa::Context asm_ctx(ISA_ASM);
  auto asm_mod = init_mod("foo");
  matcher.rewrite(*asm_mod, asm_ctx);

  asm_mod->dump(std::cout);
  std::cout << "\n";
  isa::check(asm_ctx, *asm_mod);

  return 0;
}
