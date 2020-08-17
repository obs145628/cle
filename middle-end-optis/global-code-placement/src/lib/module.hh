#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Ins {
  // All labels defined at the same location than this instruction
  std::vector<std::string> label_defs;

  std::vector<std::string> args;

  // Optional end-of-line command, enpty if none
  std::string comm_eol;

  static Ins parse(const std::string &str);

  // Only dump ins text code (no labels / comment)
  void dump(std::ostream &os) const;
};

struct Module {
  std::vector<Ins> code;

  // map label name => index in code
  std::map<std::string, std::size_t> labels;

  static Module parse(std::istream &is);

  void dump(std::ostream &os) const;
};
