#include <cstring>
#include <fstream>
#include <iostream>

#include "lib/dvnt.hh"
#include "lib/loader.hh"
#include "lib/module.hh"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: optime-dom-value-numbering <src-file>" << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  auto mod = load_module(in_file);

  dvnt_run(*mod);
  mod->check();

  auto gout = mod2gop(*mod);
  gout.dump(std::cout);
  isa::check(gout);

  return 0;
}
