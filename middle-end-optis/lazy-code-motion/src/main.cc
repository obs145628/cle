#include <cstring>
#include <fstream>
#include <iostream>

#include "lib/imodule.hh"
#include "lib/lcm.hh"
#include "lib/module.hh"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: lazy-code-motion <src-file>" << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  std::ifstream is(in_file);
  auto mod = mod2imod(Module::parse(is));
  imod2mod(*mod).dump(std::cout);
  std::cout << "\n";

  run_lcm(*mod);

  imod2mod(*mod).dump(std::cout);
  std::cout << "\n";

  return 0;
}
