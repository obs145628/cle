#include <cstring>
#include <fstream>
#include <iostream>

#include "isa/isa.hh"
#include "isa/module.hh"
#include "lib/renamer.hh"
#include "lib/sched.hh"
#include <gop10/module.hh>

#define ISA_IR (CMAKE_SRC_DIR "/config/isa_ir.txt")

int main(int argc, char **argv) {
  MDLogger::init(argc, argv);
  if (argc < 2) {
    std::cerr << "Usage: ./isched-local-list-eb <ir-file>" << std::endl;
    return 1;
  }

  // Parse input file
  isa::Context ctx_ir(ISA_IR);
  std::ifstream ifs(argv[1]);
  isa::Module ir_mod(ctx_ir, gop::Module::parse(ifs));
  ir_mod.check();

  // First rename code
  // Can remove some anti-dependencies
  Renamer renamer(ir_mod);
  renamer.run();

  // Schedule all functions one by one
  for (auto fun : ir_mod.funs()) {
    Scheduler sc(*fun);
    sc.run();
  }

  return 0;
}
