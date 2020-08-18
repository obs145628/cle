#pragma once

#include <ostream>
#include <set>
#include <string>
#include <vector>

#include <gop10/fwd.hh>

namespace isa {

constexpr std::size_t IDX_NO = -1;

using ins_t = std::vector<std::string>;
using dir_t = std::vector<std::string>;
using def_t = std::vector<std::string>;

// Return index of register def, or IDX_NO if no register defined
std::size_t def_idx(const def_t &ins);

// Get register defined
std::string get_def(const def_t &ins);

// Return list of index that are used registers
std::vector<std::size_t> uses_idxs(const def_t &ins);

// Get all registers used by ins
std::set<std::string> get_uses(const def_t &ins);

std::set<std::string> get_regs(const def_t &ins);

std::vector<std::string> get_labels(const def_t &ins);

void replace_regs(def_t &ins, const std::string &old_reg,
                  const std::string &new_reg);

void replace_labels(def_t &ins, const std::string &old_label,
                    const std::string &new_label);

// Is an instruction that may change control flow (call doesn't count)
bool is_branch(const def_t &ins);

// Return list of all target labels for a branch instruction
std::vector<std::size_t> branch_targets_idxs(const def_t &ins);
std::vector<std::string> branch_targets(const def_t &ins);

// Is it a call instruction
bool is_call(const def_t &ins);

// Does the call expect a return value
bool call_has_ret(const def_t &ins);

std::string call_get_function_name(const def_t &ins);

// Return index of first arg
std::size_t call_args_beg(const def_t &ins);

// Return 1 + index of last arg
std::size_t call_args_end(const def_t &ins);

std::size_t call_args_count(const def_t &ins);

// Return index of matching label, or IDX_NO if not found
std::size_t phi_find_label(const def_t &ins, const std::string &label);

std::vector<std::string> phi_get_labels(const def_t &ins);

bool fundecl_has_ret(const def_t &dir);

std::size_t fundecl_args_beg(const def_t &dir);

std::size_t fundecl_args_end(const def_t &dir);

std::size_t fundecl_args_count(const def_t &dir);

// Return the list of registers that are arguments
std::vector<std::string> fundecl_args(const def_t &dir);

void fundecl_rename_arg(def_t &dir, std::size_t idx,
                        const std::string &new_arg);

void dump(std::ostream &os, const def_t &ins);

// Check if an instruction is valid
// Syntax test, no context info
// Panic if any error is found
void check_ins(const def_t &ins);

// Check if a module is valid
// Panic if any error is found
void check(const gop::Module &mod);

} // namespace isa
