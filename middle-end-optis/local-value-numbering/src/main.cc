#include "lib/lvn.hh"
#include "lib/lvn_ext.hh"
#include "lib/module.hh"

#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: test-optime-local-value-numbering <src-file> "
                 "[--ext]"
              << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  auto use_ext = argc > 2 && strcmp(argv[2], "--ext") == 0;

  std::ifstream is(in_file);
  auto mod = parse_mod(is);

  if (use_ext)
    run_lvn_ext(mod);
  else
    run_lvn(mod);

  dump_mod(std::cout, mod);

  return 0;
}
