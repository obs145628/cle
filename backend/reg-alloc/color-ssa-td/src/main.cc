#include <cstring>
#include <fstream>
#include <iostream>

#include "isa/isa.hh"
#include "isa/module.hh"
#include "lib/allocator.hh"
#include <gop10/module.hh>
#include <mdlogger/mdlogger.hh>

#define ISA_IR (CMAKE_SRC_DIR "/config/isa_ir.txt")

void dump_mod(const std::string &title, const isa::Module &mod) {
  auto doc = MDLogger::instance().add_doc(title);
  mod.dump_code(*doc);
  mod.dump_code(std::cout);
  mod.check();
}

int main(int argc, char **argv) {
  MDLogger::init(argc, argv);
  if (argc < 3) {
    std::cerr << "Usage: ./ralloc-col-ssa-td <ir-file> <hr-count>" << std::endl;
    return 1;
  }

  // Parse input file
  isa::Context ctx_ir(ISA_IR);
  std::ifstream ifs(argv[1]);
  isa::Module ir_mod(ctx_ir, gop::Module::parse(ifs));
  dump_mod("Input IR", ir_mod);

  // Perform register allocation
  Allocator::apply(ir_mod, std::atoi(argv[2]));

  dump_mod("Output IR", ir_mod);

  return 0;
}
