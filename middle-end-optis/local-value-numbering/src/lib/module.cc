#include "module.hh"

#include <utils/str/str.hh>

Module parse_mod(std::istream &is) {
  Module mod;

  std::string line;
  while (std::getline(is, line)) {
    line = utils::str::trim(line);
    if (line.empty())
      continue;

    Ins ins;
    ins.args = utils::str::split(line, ' ');
    mod.code.push_back(ins);
  }

  return mod;
}

void dump_mod(std::ostream &os, const Module &mod) {
  for (const auto &ins : mod.code) {
    for (const auto &arg : ins.args)
      os << arg << " ";
    os << "\n";
  }
}
