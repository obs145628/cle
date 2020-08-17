#include <cstring>
#include <fstream>
#include <iostream>

#include "lib/gop.hh"
#include "lib/module-load.hh"
#include "lib/module.hh"
#include "lib/pp.hh"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: optime-procedure-placement <src-file>" << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  std::vector<CallInfos> ci;
  auto mod = load_module(in_file, ci);

  pp_run(*mod, ci);

  mod2gop(*mod).dump(std::cout);
  return 0;
}
