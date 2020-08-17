#include <cstring>
#include <fstream>
#include <iostream>

#include "lib/imodule.hh"
#include "lib/liveout.hh"
#include "lib/module.hh"

//
// Find potential uses of register values before definition
// Compute Liveout(entry), with entry special entry BB that has one ins: jump to
// true entry
// Liveout(entry) contains all regs r for wich it exists a path in a program
// such that r is used before being redefined
// r couldn't be defined in entry because it's only a jump, and it has no
// prodecessor, so it means it may be used before definition
// It's only a possibily, because the path may be infeasible

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: test-optime-uninit-regs <src-file>" << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  std::ifstream is(in_file);
  auto mod = mod2imod(Module::parse(is));
  imod2mod(*mod).dump(std::cout);
  std::cout << "\n";

  auto liveout = run_liveout(*mod);
  auto entry_liveout = liveout[&mod->get_entry_bb()];
  for (auto r : entry_liveout)
    std::cout << "Warning: register " << r
              << " may be used before intialization\n";

  return 0;
}
