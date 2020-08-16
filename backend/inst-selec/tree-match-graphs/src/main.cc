#include <cstring>
#include <fstream>
#include <iostream>

#include "isa/isa.hh"
#include "lib/block-graph.hh"
#include "lib/context.hh"
#include "lib/matcher.hh"
#include "lib/module.hh"
#include "lib/rules.hh"
#include <gop10/module.hh>
#include <logia/program.hh>

std::unique_ptr<gop::Module> init_mod(const std::string &fun_name) {
  auto mod = std::make_unique<gop::Module>();
  auto fun = std::make_unique<gop::Dir>(std::vector<std::string>{"fun"});
  fun->label_defs.push_back(fun_name);
  mod->decls.push_back(std::move(fun));
  return mod;
}

// Tree-Matching implementation
// Second trial of implementation
// Handle multiple basic blocks, non-SSA regs, and digraph
//
// Inspired from:
// Instruction Selection via Tree-Pattern Matching - Enginner a Compiler p610

int main(int argc, char **argv) {
  logia::Program::set_command(argc, argv);
  if (argc < 3) {
    std::cerr << "Usage: ./iselec-tree-match-graphs <arch> <ir-file>"
              << std::endl;
    return 1;
  }

  // Init backend
  auto ctx = Context::make(argv[1]);
  ctx->init();
  auto &rules = ctx->rules();
  (void)rules;

  // Parse input file
  std::ifstream ifs(argv[2]);
  auto gmod = gop::Module::parse(ifs);
  isa::check(ctx->ir_ctx(), gmod);
  Module ir_mod(ctx->ir_ctx(), gmod);
  ir_mod.dump_code(std::cout);

  {
    auto log = logia::Program::instance().add_doc<logia::MdGfmDoc>("Input IR");
    ir_mod.dump_code(*log);
  }

  // Preprocess input IR
  // Perform arch-dependant analyses and transform on the IR
  ctx->preprocess_ir(ir_mod);

  {
    auto log = logia::Program::instance().add_doc<logia::MdGfmDoc>(
        "IR After preprocess");
    ir_mod.dump_code(*log);
  }

  Module asm_mod(ctx->asm_ctx());

  for (auto ir_fun : ir_mod.funs()) {

    auto asm_fun = asm_mod.add_fun(ir_fun->name(), {});

    for (auto ir_bb : ir_fun->bbs()) {
      // Create a digraph representing the block instructions
      BlockGraph bg(*ir_bb);

      // Convert graph to a forest of trees
      bg.to_trees();

      // Convert each tree to ASM code
      auto asm_bb = asm_fun->add_block(ir_bb->name());
      for (auto tree : bg.get_roots()) {
        Matcher matcher(rules, bg, *tree);
        matcher.run();
        matcher.rewrite(*asm_bb);
      }
    }
  }

  {
    auto log = logia::Program::instance().add_doc<logia::MdGfmDoc>(
        "Instruction Selection Output");
    asm_mod.dump_code(*log);
  }

  // Apply a platform-dependant pass on the generated ASM
  ctx->update_asm(asm_mod);

  {
    auto log =
        logia::Program::instance().add_doc<logia::MdGfmDoc>("Output ASM");
    asm_mod.dump_code(*log);
  }

  asm_mod.dump_code(std::cout);
  return 0;
}
