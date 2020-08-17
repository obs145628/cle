#pragma once

#include "../isa/module.hh"

#include <logia/md-gfm-doc.hh>

// Top-Down Local Register Allocation
// Perform register allocation for a single basic block
// Assign one reg for the whole basic black duration to every virtual reg
// Perform register spilling with the statck if no hardware registers left
//
// Algorithm Top-Down Local Register Allocation - Engineer a Compiler p685

// Hardware registers: h0 -> h (hr_count - 1) + hsp
// Argument registers: args0 -> args3 : always mapped to h0 -> h3
// Return register: ret mapped to h0
// Stack register sp: mapped to hsp
//
// First arg regs are mapped to corresponding h0 to h3 max regs
// Then maps other regs to following h<x>, depending on frequency
// Spill remaining regs to memory
// For ret, it gets mapped to h0 if arg0 doesn't exist
// Otherwhise, it gets spilled and reloaded right before every ret

class Allocator {

public:
  Allocator(isa::BasicBlock &bb, std::size_t hr_count);

  static void apply(isa::Module &mod, std::size_t hr_count);

  void run();

private:
  isa::BasicBlock &_bb;
  const isa::Context &_ctx;
  std::size_t _hr_count;
  std::map<std::string, std::string> _special;
  std::map<std::string, std::size_t> _freq;
  std::vector<std::string> _sorted;
  std::map<std::string, std::string> _mapped;
  std::map<std::string, std::size_t> _spilled;
  std::size_t _spill_count;
  std::size_t _spill_start_addr;
  std::size_t _spill_next_tmp;

  std::size_t _args_count; // number of arg regs used
  std::size_t
      _virt_count; // number of virtual regs used (doesn't count special regs)

  std::vector<std::vector<std::string>>
      _code_pre; // Code inserted before current ins
  std::vector<std::vector<std::string>>
      _code_post; // Code inserted after current ins

  std::unique_ptr<logia::MdGfmDoc> _doc;

  void _init();

  void _compute_freq();
  void _choose_hard_regs();
  void _choose_spills();

  std::string _get_tmp_spill();

  void _rewrite();
  std::vector<std::vector<std::string>>
  _rewrite_ins(const std::vector<std::string> &ins);
  std::string _rewrite_use(const std::string &arg);
  std::string _rewrite_def(const std::string &arg);
};
