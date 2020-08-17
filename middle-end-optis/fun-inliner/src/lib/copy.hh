
#pragma once

#include <map>
#include <string>

#include "module.hh"

// Describe transformations done when cloning instruction
struct CloneMap {
  // map old => new reg name
  std::map<std::string, std::string> regs;

  // map old => new bb name (for branching ins)
  std::map<std::string, std::string> bbs;

  // map old => new fun name (for call ins)
  std::map<std::string, std::string> funs;
};

// Clone instruction `ins`
// new cloned instruction inserted in `out_bb` at  `insert_point`
void clone_ins(const Instruction &ins, const CloneMap &cm, BasicBlock &out_bb,
               ins_iterator_t insert_point);

// Clone basic block `bb` into out_bb at `insert_point`
void clone_bb_into(const BasicBlock &bb, BasicBlock &out_bb,
                   ins_iterator_t insert_point, const CloneMap &cm);

// Clone all basic block of bb
// Cloned basic blocks are inserted into out_fun at out_it
// if remap_new_bbs is true, also use the mapping fun bb's => new bbs names
// Return iterator to first bb inserted
bb_iterator_t clone_fun_into(const Function &fun, Function &out_fun,
                             bb_iterator_t out_it, const CloneMap &cm,
                             bool remap_new_bbs);

// Replace all uses of old_val operand by new_val operand
void replace_all_ops(const std::string &old_val, const std::string &new_val,
                     BasicBlock &bb);
void replace_all_ops(const std::string &old_val, const std::string &new_val,
                     Function &fun);
void replace_all_ops(const std::string &old_val, const std::string &new_val,
                     Module &mod);
