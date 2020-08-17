#pragma once

#include <iostream>
#include <string>
#include <vector>

struct Ins {
  std::vector<std::string> args;
};

struct Module {
  std::vector<Ins> code;
};

Module parse_mod(std::istream &is);

void dump_mod(std::ostream &os, const Module &mod);
