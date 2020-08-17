#include <cstring>
#include <fstream>
#include <iostream>

#include "lib/gcp.hh"
#include "lib/imodule.hh"
#include "lib/module.hh"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: global-code-placement <src-file>" << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  std::ifstream is(in_file);
  auto mod = mod2imod(Module::parse(is));

  gcp_run(*mod);

  imod2mod(*mod).dump(std::cout);

  return 0;
}
