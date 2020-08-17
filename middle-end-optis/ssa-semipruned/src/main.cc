#include <cstring>
#include <fstream>
#include <iostream>

#include "lib/gop.hh"
#include "lib/module-load.hh"
#include "lib/module.hh"
#include "lib/ssa.hh"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: ssa-semipruned <src-file>" << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  auto mod = load_module(in_file);

  ssa_run(*mod);

  mod2gop(*mod).dump(std::cout);
  return 0;
}
