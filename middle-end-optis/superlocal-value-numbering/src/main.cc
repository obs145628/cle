#include <cstring>
#include <fstream>
#include <iostream>

#include "lib/module.hh"
#include "lib/slvn.hh"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: superlocal-value-numbering <src-file>" << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  std::ifstream is(in_file);
  auto mod = Module::parse(is);

  run_slvn(mod);

  mod.dump(std::cout);
  return 0;
}
