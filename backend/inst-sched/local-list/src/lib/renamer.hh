#pragma once

#include "../isa/isa.hh"
#include "../isa/module.hh"

class Renamer {
public:
  Renamer(isa::Module &mod);

  void run();

private:
  isa::Module &_mod;
  const isa::Context &_ctx;

  void _udp_bb(isa::BasicBlock &bb);

  // Rename all uses of old_name by new_names starting at ins #beg
  // stops when old_name is redefined
  void _rename_uses(isa::BasicBlock &bb, const std::string &old_name,
                    const std::string &new_name, std::size_t beg);

  void _rename_def(std::vector<std::string> &ins, const std::string &new_name);
  void _rename_uses(std::vector<std::string> &ins, const std::string &old_name,
                    const std::string &new_name);
};
