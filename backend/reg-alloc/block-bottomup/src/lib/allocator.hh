#pragma once

#include "../isa/module.hh"

#include <logia/md-gfm-doc.hh>

// Bottum-Up Local Register Allocation
// Perform register allocation for a single basic block
// Keep stack of the state of every virtual register: stored in memory, or in a
// physical register, and a list of free regs.
// Goes through ins in order, and for each, ensure operands are in a physical
// reg, by allocating a free reg and moving value to this reg if necessary.
// If no free reg available, it must spill a used one. It takes the one that is
// used next the latest.
// After operands are loaded, if regs are not used anymore, they are marked as
// free.
//
// To known when values become dead, and when the next use will be can be
// precomputed in a single backward pass over the block, and stored in a table.
//
// Algorithm Bottom-Up Local Register Allocation - Engineer a Compiler p686

// Hardware registers: h0 -> h (hr_count - 1) + hsp
// Argument registers: arg0 -> arg3 : always mapped to h0 -> h3
// Return register: ret mapped to h0
// Stack register sp: mapped to hsp
//
//

class Allocator {

  struct HardReg {
    std::size_t idx;
    std::string virt; // name of virtual reg using it, or empty if free
    std::size_t next; // index of next instruction that use this reg,
    // or -1 if no next use
  };

  struct RegState {
    std::string name;
    std::size_t hr_idx; // hard reg index, or -1 if spilled
    std::size_t sp_off; // stack offset, or -1 if in reg
  };

public:
  Allocator(isa::BasicBlock &bb, std::size_t hr_count);

  static void apply(isa::Module &mod, std::size_t hr_count);

  void run();

private:
  isa::BasicBlock &_bb;
  const isa::Context &_ctx;
  std::size_t _hr_count;

  std::map<std::string, std::size_t> _special_regs;

  std::size_t _ins_idx;
  std::vector<std::map<std::string, std::size_t>> _next_uses;
  std::set<std::size_t> _blocked_uses;
  std::vector<HardReg> _hard_regs;
  std::vector<std::size_t> _free_regs;
  std::map<std::string, RegState> _vstates;
  std::vector<std::vector<std::string>>
      _extra_code; // code to get values in right register

  std::unique_ptr<logia::MdGfmDoc> _doc;

  std::vector<std::size_t> _spill_locs;

  void _init();

  void _prepare_args();
  void _compute_next_uses();

  void _rewrite();
  std::vector<std::vector<std::string>> _rewrite_ins();

  std::size_t _ensure(const std::string &vreg);
  void _ensure_fixed(const std::string &vreg, std::size_t reg_id);
  std::string _ensure_use(const std::string &vreg);

  std::size_t _alloc(const std::string &vreg);
  void _alloc_fixed(const std::string &vreg, std::size_t reg_id);
  std::string _alloc_def(const std::string &vreg, std::size_t reg_hint);

  std::size_t _select_spill();
  void _spill_reg(std::size_t reg_id);

  void _free(const std::string &vreg);
  void _free_reg(std::size_t reg_id);
};
