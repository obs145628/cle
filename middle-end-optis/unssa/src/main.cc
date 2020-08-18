#include <cstring>
#include <fstream>
#include <iostream>

#include "lib/critical.hh"
#include "lib/loader.hh"
#include "lib/module.hh"
#include "lib/unssa.hh"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: unssa <src-file>" << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  auto mod = load_module(in_file);

  critical_split(*mod);
  mod->check();
  auto gout = unssa(*mod);

  gout.dump(std::cout);
  isa::check(gout);

  return 0;
}
