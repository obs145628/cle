#pragma once

#include "../isa/isa.hh"
#include "../isa/module.hh"
#include "live-out.hh"

class Renamer {
public:
  Renamer(isa::Module &mod);

  void run();

private:
  isa::Module &_mod;
  const isa::Context &_ctx;
  std::map<const isa::Function *, const LiveOut *> _liveouts;
  std::size_t _next_reg;

  void _udp_bb(isa::BasicBlock &bb);

  // Rename all uses of old_name by new_names starting at ins #beg
  // stops when old_name is redefined
  // Can only rename variables that aren't in live at the end of BB
  // Actually even those could be renamed, the only case that prevents renaming
  // is when there is a back-edge, causing the variable to be used before its
  // definition
  // These usually are loop variables that are changed  in loop body
  void _rename_uses(isa::BasicBlock &bb, const std::string &old_name,
                    const std::string &new_name, std::size_t beg);

  void _rename_def(std::vector<std::string> &ins, const std::string &new_name);
  void _rename_uses(std::vector<std::string> &ins, const std::string &old_name,
                    const std::string &new_name);

  std::string _gen_reg_name();
};
