#include <cstring>
#include <fstream>
#include <iostream>

#include "isa/isa.hh"
#include "isa/module.hh"
#include "lib/sched.hh"
#include <gop10/module.hh>
#include <logia/md-gfm-doc.hh>
#include <logia/program.hh>

#define ISA_IR (CMAKE_SRC_DIR "/config/isa_ir.txt")

int main(int argc, char **argv) {
  logia::Program::set_command(argc, argv);
  if (argc < 2) {
    std::cerr << "Usage: ./isched-local-list <ir-file>" << std::endl;
    return 1;
  }

  // Parse input file
  isa::Context ctx_ir(ISA_IR);
  std::ifstream ifs(argv[1]);
  isa::Module ir_mod(ctx_ir, gop::Module::parse(ifs));

  // Run scheduler
  Scheduler sc(ir_mod);
  sc.run();

  return 0;
}
