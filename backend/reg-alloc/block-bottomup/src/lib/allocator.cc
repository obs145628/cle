#include "allocator.hh"

#include <algorithm>
#include <cassert>
#include <cstring>

#include <logia/program.hh>

namespace {

constexpr std::size_t IDX_NONE = -1;

constexpr std::size_t SP_OFF_START = 4;
constexpr std::size_t SP_ELEM_SIZE = 4;
constexpr std::size_t ARGS_MAX = 4;

std::string hr_arg(std::size_t id) { return "%h" + std::to_string(id); }

// bool is_reg(const std::string &str) { return str.front() == '%'; }

bool is_arg_reg(const std::string &str) {
  return str.size() == 5 && std::strncmp(str.c_str(), "%arg", 4) == 0 &&
         str[4] >= '0' && str[4] <= ('0' + int(ARGS_MAX) - 1);
}

void dump_ins(std::ostream &os, const std::vector<std::string> &ins) {
  os << "    ";
  for (std::size_t i = 0; i < ins.size(); ++i) {
    if (i == 1)
      os << " ";
    else if (i > 1)
      os << ", ";
    os << ins[i];
  }
}

} // namespace

Allocator::Allocator(isa::BasicBlock &bb, std::size_t hr_count)
    : _bb(bb), _ctx(_bb.parent().parent().ctx()), _hr_count(hr_count) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(
      "Allocator for @" + bb.parent().name() + ":@" + bb.name());

  _init();
}

void Allocator::apply(isa::Module &mod, std::size_t hr_count) {
  for (auto fun : mod.funs()) {
    assert(fun->bbs().size() == 1);
    Allocator alloc(*fun->bbs().front(), hr_count);
    alloc.run();
  }
}

void Allocator::run() {

  // Compute liveliness infos
  _compute_next_uses();

  // Setup registers for arguments
  _prepare_args();

  // Rewrite code
  _rewrite();
}

void Allocator::_init() {
  _ins_idx = 0;

  for (std::size_t i = 0; i < ARGS_MAX; ++i)
    _special_regs["arg" + std::to_string(i)] = i;
  _special_regs["ret"] = 0;

  for (std::size_t i = 0; i < _hr_count; ++i) {
    HardReg reg;
    reg.idx = i;
    reg.virt = "";
    reg.next = IDX_NONE;
    _hard_regs.push_back(reg);
    _free_regs.insert(_free_regs.begin(), i);
  }
}

void Allocator::_prepare_args() {
  int used[ARGS_MAX] = {0, 0, 0, 0};

  for (const auto &ins : _bb.code())
    for (const auto &arg : ins)
      if (is_arg_reg(arg))
        used[arg[4] - '0'] = 1;

  for (std::size_t i = 0; i < ARGS_MAX; ++i)
    if (used[i]) {
      auto vreg = "arg" + std::to_string(i);
      _alloc_fixed(vreg, i);
    }
}

// _next_uses[idx][vreg] = next_id means that before instruction idx, vreg will
// be in use next at instruction next_idx
// if vreg not reused, or killed before being reused, vreg not in
// _next_uses[idx]
void Allocator::_compute_next_uses() {

  // Size + 1 to avoid bound check for last instruction
  _next_uses.resize(_bb.code().size() + 1);

  std::map<std::string, std::size_t> actual;

  for (std::size_t ins_idx = _bb.code().size() - 1; ins_idx < _bb.code().size();
       --ins_idx) {
    isa::Ins ins(_ctx, _bb.code()[ins_idx]);
    for (const auto &r : ins.args_uses())
      actual[r] = ins_idx;
    for (const auto &r : ins.args_defs())
      actual.erase(r);

    _next_uses[ins_idx] = actual;
  }

  *_doc << "## Next uses\n";
  auto ch = _doc->code("asm");

  for (std::size_t i = 0; i < _bb.code().size(); ++i) {
    _bb.dump_ins(ch.os(), i);
    ch << "; [" << i << "] { ";
    for (auto it : _next_uses[i])
      ch << "%" << it.first << ": " << it.second << ", ";
    ch << "}\n";
  }
}

// Rewrite all instructions and update BB
void Allocator::_rewrite() {
  std::vector<std::vector<std::string>> new_code;

  for (_ins_idx = 0; _ins_idx < _bb.code().size(); ++_ins_idx) {
    *_doc << "## Rewrite ins " << _ins_idx << "\n";

    {
      auto ch = _doc->code("asm");
      _bb.dump_ins(ch.os(), _ins_idx);
      ch << "\n";
    }

    auto ins_code = _rewrite_ins();
    new_code.insert(new_code.end(), ins_code.begin(), ins_code.end());

    {
      auto ch = _doc->code("asm");
      for (const auto &x : ins_code) {
        dump_ins(ch.os(), x);
        ch << "\n";
      }
    }
  }

  _bb.code() = new_code;
}

// Rewrite code for current instruction
// Add extra instructions needed for spilling
std::vector<std::vector<std::string>> Allocator::_rewrite_ins() {
  isa::Ins ins(_ctx, _bb.code()[_ins_idx]);
  _extra_code.clear();
  auto new_ins = ins.args();
  auto defs = ins.args_defs();

  // Special case for ret instruction
  // Need to be sure ret value is in h0
  if (ins.opname() == "ret")
    _ensure_fixed("ret", 0);

  // Already add regs of current regs to blocked uses
  // This is to avoid op #1 spilling op #2
  for (const auto &r : ins.args_uses()) {
    if (r == "sp")
      continue;
    const auto &state = _vstates.at(r);
    if (state.hr_idx != IDX_NONE)
      _blocked_uses.insert(state.hr_idx);
  }

  // First rewrite uses
  for (std::size_t i = 0; i < ins.args().size() - 1; ++i)
    if (ins.get_arg_kind(i) == isa::ARG_KIND_USE) {
      auto vreg = ins.args()[i + 1].substr(1);
      new_ins[i + 1] = "%" + _ensure_use(vreg);
    }

  // Free all last uses
  // It shouldn't be done in the rewrite use loop because one reg may be used in
  // multiple operands
  // It shouldn't be done after the def because the free reg could be save one
  // spill for the def
  for (const auto &vreg : ins.args_uses()) {
    if (vreg == "sp")
      continue; // ignore stack pointer
    if (_next_uses[_ins_idx + 1].count(vreg) == 0 ||
        std::find(defs.begin(), defs.end(), vreg) != defs.end()) {
      // Also need to check defs, because the reg value might be killed by
      // def, and then still be there in _next_uses of next ins.
      // In this case the reg still need to be free because the value won't be
      // used
      _free(vreg);
    }
  }

  // Then rewrite defs
  for (std::size_t i = 0; i < ins.args().size() - 1; ++i)
    if (ins.get_arg_kind(i) == isa::ARG_KIND_DEF) {

      auto reg_hint = IDX_NONE;
      if (ins.opname() == "mov") {
        // For mov instruction, try to allocate the same reg for def and use
        // it will generate the code mov %h<x>, %h<x>, which as no effect, and
        // can be skipped
        auto &src_state = _vstates.at(ins.args()[2].substr(1));
        if (src_state.hr_idx != IDX_NONE)
          reg_hint = src_state.hr_idx;
      }

      auto vreg = ins.args()[i + 1].substr(1);
      new_ins[i + 1] = "%" + _alloc_def(vreg, reg_hint);
    }

  // Instruction is finished, no regs are blocked anymore
  _blocked_uses.clear();

  auto res = _extra_code;
  res.push_back(new_ins);

  // Special case: mov %h<x>, %h<x> is useless and can be ignored
  if (new_ins[0] == "mov" && new_ins[1] == new_ins[2])
    res.erase(res.end() - 1);

  return res;
}

// Ensure the value in vreg is stored to a register
// Insert code to move the value if needed
// This is called only for uses, not defs
std::size_t Allocator::_ensure(const std::string &vreg) {
  auto &state = _vstates.at(vreg);

  if (state.hr_idx != IDX_NONE) {
    // Already in a register
    return state.hr_idx;
  }

  // Trigger means using dead value, invalid asm code
  assert(state.sp_off != IDX_NONE);

  auto sp_off = state.sp_off;
  auto reg_idx = _alloc(vreg);
  _extra_code.push_back(
      {std::string{"load"}, hr_arg(reg_idx), "%hsp", std::to_string(sp_off)});
  return reg_idx;
}

// Ensure the value in vreg is stored to a register
// Insert code to move the value if needed
// This is called only for uses, not defs
// Panics if the value is stored in a different register
// Should always be called with same reg_id for same vreg
void Allocator::_ensure_fixed(const std::string &vreg, std::size_t reg_id) {
  auto &state = _vstates.at(vreg);

  if (state.hr_idx != IDX_NONE) {
    // Already in a register
    assert(state.hr_idx == reg_id);
    return;
  }

  // Trigger means using dead value, invalid asm code
  assert(state.sp_off != IDX_NONE);

  auto sp_off = state.sp_off;
  _alloc_fixed(vreg, reg_id);
  _extra_code.push_back(
      {std::string{"load"}, hr_arg(reg_id), "%hsp", std::to_string(sp_off)});
}

// Wrapper around ensure / ensure_fixed
// Used for all use regs
// Make sure special regs are always stored in the right location
// Return name of hardware reg
std::string Allocator::_ensure_use(const std::string &vreg) {
  if (vreg == "sp")
    return "hsp";

  auto it = _special_regs.find(vreg);
  if (it != _special_regs.end()) {
    // Can only allocate to fixed register
    auto reg_id = it->second;
    _ensure_fixed(vreg, reg_id);
    _blocked_uses.insert(reg_id);
    return "h" + std::to_string(reg_id);
  }

  auto reg_id = _ensure(vreg);
  _blocked_uses.insert(reg_id);
  return "h" + std::to_string(reg_id);
}

// Allocate a register for vreg
// If none is available, spill one already used
// This is called only for defs, not uses
// @TODO: spill the one with bigger next
std::size_t Allocator::_alloc(const std::string &vreg) {
  if (_free_regs.empty()) {
    // Spill a register
    auto reg_id = _select_spill();
    _spill_reg(reg_id);
  }

  auto reg_id = _free_regs.back();
  _alloc_fixed(vreg, reg_id);
  return reg_id;
}

// Allocates a fixed register
// Value is spilled to memory if this register is already used
void Allocator::_alloc_fixed(const std::string &vreg, std::size_t reg_id) {
  auto &hr = _hard_regs.at(reg_id);
  if (!hr.virt.empty()) {
    // Register already used, spill to memory
    _spill_reg(reg_id);
  }

  auto it = std::find(_free_regs.begin(), _free_regs.end(), reg_id);
  assert(it != _free_regs.end());
  _free_regs.erase(it);
  hr.virt = vreg;
  // @TODO: next ?

  RegState state;
  state.name = vreg;
  state.sp_off = IDX_NONE;
  state.hr_idx = reg_id;
  _vstates[state.name] = state;

  *_doc << "- [" << _ins_idx << "]: alloc %" << vreg << " => "
        << "%h" << reg_id << "\n";
}

// Wrapper around alloc / alloc_def
// Ensures special regs are correctly allocated
// Panics if vreg is not dead
// if reg_hint != ID_NONE, and reg_hint is free, use it
std::string Allocator::_alloc_def(const std::string &vreg,
                                  std::size_t reg_hint) {
  if (vreg == "sp")
    return "hsp";

  {
    auto it = _vstates.find(vreg);
    assert(it == _vstates.end() ||
           (it->second.hr_idx == IDX_NONE && it->second.sp_off == IDX_NONE));
  }

  auto it = _special_regs.find(vreg);
  if (it != _special_regs.end()) {
    // Allocate fixed register
    auto reg_id = it->second;
    _alloc_fixed(vreg, reg_id);
    return "h" + std::to_string(reg_id);
  }

  if (reg_hint != IDX_NONE && _hard_regs[reg_hint].virt.empty()) {
    _alloc_fixed(vreg, reg_hint);
    return "h" + std::to_string(reg_hint);
  }

  auto reg_id = _alloc(vreg);
  return "h" + std::to_string(reg_id);
}

// Select the best register to spill
// 1) Some registers cannot be spilled because they are in use by the current
// ins
// 2) Otherwhise select the one which will be used on the latest
// There shouldn't be a used reg that if dead I suppose rule 2 could also make 1
// works, except in weird cases when they are less operand regs than actual regs
std::size_t Allocator::_select_spill() {

  std::size_t best_reg = IDX_NONE;
  std::size_t best_next = 0;

  for (std::size_t reg_id = 0; reg_id < _hr_count; ++reg_id) {
    const auto &reg = _hard_regs.at(reg_id);
    if (reg.virt.empty()) // already free
      continue;

    if (_blocked_uses.count(reg_id)) // already used by current instruction
      continue;

    std::size_t next = _next_uses[_ins_idx + 1].at(reg.virt);
    if (next >= best_next) {
      best_reg = reg_id;
      best_next = next;
    }
  }

  assert(best_reg != IDX_NONE);
  return best_reg;
}

// Spill the register to memory, and free reg
void Allocator::_spill_reg(std::size_t reg_id) {
  auto &reg = _hard_regs.at(reg_id);
  auto &state = _vstates.at(reg.virt);

  // Cannot spill a register currently used by an instruction
  assert(_blocked_uses.count(reg_id) == 0);

  // Find free spill position
  std::size_t pos = 0;
  while (pos < _spill_locs.size() && _spill_locs[pos])
    ++pos;
  if (pos == _spill_locs.size())
    _spill_locs.push_back(0);
  _spill_locs[pos] = 1;
  std::size_t sp_off = SP_OFF_START + (SP_ELEM_SIZE * pos);

  // Add code to write value to memory
  _extra_code.push_back({std::string{"store"}, hr_arg(reg_id),
                         std::string("%hsp"), std::to_string(sp_off)});

  // Mark reg free
  _free_reg(reg_id);

  // Update reg state
  state.hr_idx = IDX_NONE;
  state.sp_off = sp_off;

  *_doc << "- [" << _ins_idx << "]: spill %" << state.name << " (%h" << reg_id
        << ") => "
        << "%hsp + " << sp_off << "\n";
}

// Mark this vreg as unused
// Free it's hardware register if one
// Otherwhise free spill slot on stack
void Allocator::_free(const std::string &vreg) {
  auto &state = _vstates.at(vreg);
  if (state.hr_idx != IDX_NONE) {
    *_doc << "- [" << _ins_idx << "]: free %" << vreg << " (%h" << state.hr_idx
          << ")\n";

    _free_reg(state.hr_idx);
    state.hr_idx = IDX_NONE;
  }

  else if (state.sp_off != IDX_NONE) {
    *_doc << "- [" << _ins_idx << "]: free %" << vreg << " (%hsp + "
          << state.sp_off << ")\n";

    auto pos = (state.sp_off - SP_OFF_START) / SP_ELEM_SIZE;
    assert(_spill_locs.at(pos) == 1);
    _spill_locs[pos] = 0;
    state.sp_off = IDX_NONE;
  }

  else
    assert(0);
}

// Mark this register as free
void Allocator::_free_reg(std::size_t reg_id) {
  auto &reg = _hard_regs.at(reg_id);
  assert(!reg.virt.empty());
  reg.virt.clear();
  reg.next = IDX_NONE;

  _free_regs.push_back(reg_id);
}
