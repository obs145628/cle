#pragma once

#include <cassert>
#include <map>
#include <memory>
#include <ostream>
#include <vector>

#include "iterators.hh"
#include "names-table.hh"
#include "ptr_list.hh"
#include "value.hh"

#include "../isa/isa.hh"

class Instruction;
class BasicBlock;
class Function;
class Module;

using ins_iterator_t = PtrList<Instruction>::iterator_t;
using const_ins_iterator_t = PtrList<Instruction>::const_iterator_t;
using bb_iterator_t = PtrList<BasicBlock>::iterator_t;
using const_bb_iterator_t = PtrList<BasicBlock>::const_iterator_t;
using fun_iterator_t = PtrList<Function>::iterator_t;
using const_fun_iterator_t = PtrList<Function>::const_iterator_t;

class Instruction : public PtrListNode<Instruction>, public Value {

public:
  BasicBlock &parent() { return *_parent; }
  const BasicBlock &parent() const { return *_parent; }

  // Completely erase instruction from its parent basic block
  void erase_from_parent();

  // Syntax check of Instruction
  // Panic if invalid
  void check() const;

  const std::string &get_opname() const;

  std::vector<std::string> sargs() const;

  bool is_branch() const;
  std::vector<BasicBlock *> branch_targets();
  std::vector<const BasicBlock *> branch_targets() const;

  // Return true if instruction outputs a value
  bool has_def() const;

  std::string to_arg() const override;

  void dump(std::ostream &os) const override;

  Instruction(const std::string &opname, const std::vector<Value *> &ops,
              const std::string &name, std::size_t def_idx, BasicBlock &parent);

private:
  BasicBlock *_parent;
  std::string _opname;
  std::size_t _def_idx;

  // Infos computed from ISA
  bool _is_branch;
  std::vector<std::size_t> _targets_idx;

  void _build_isa_infos();

  friend class BasicBlock;
};

class BasicBlock : public PtrListNode<BasicBlock>, public Value {

public:
  BasicBlock(const BasicBlock &) = delete;
  BasicBlock &operator=(const BasicBlock &) = delete;
  ~BasicBlock();

  Function &parent() { return _fun; }
  const Function &parent() const { return _fun; }

  ins_iterator_t ins_begin() { return _ins.begin(); }
  const_ins_iterator_t ins_begin() const { return _ins.begin(); }
  ins_iterator_t ins_end() { return _ins.end(); }
  const_ins_iterator_t ins_end() const { return _ins.end(); }

  IteratorRange<ins_iterator_t> ins() {
    return IteratorRange<ins_iterator_t>(ins_begin(), ins_end());
  }
  IteratorRange<const_ins_iterator_t> ins() const {
    return IteratorRange<const_ins_iterator_t>(ins_begin(), ins_end());
  }

  // std::vector<Instruction> &ins() { return _ins; }
  // const std::vector<Instruction> &ins() const { return _ins; }

  // Check if a basic block is valid.
  // must respect all these rules:
  // - a bb must have at least one instruction
  // - a not-last instruction in a bb must not branch
  // - the last instruction in a bb must branch
  // - a branching must be to the beginning of a basic block
  void check() const;

  // Add a new instruction somewhere in the basic block
  ins_iterator_t insert_ins(ins_iterator_t it, const std::string &opname,
                            const std::vector<Value *> &ops,
                            const std::string &name, std::size_t def_idx) {
    return _ins.emplace(it, opname, ops, name, def_idx, *this);
  }

  // Erase completely an instruction from this basic block
  // Return iterator to next instruction
  ins_iterator_t erase_ins(ins_iterator_t it) { return _ins.erase(it); }

  // Completely erase this basick block from the parent function
  void erase_from_parent();

  // Move instructions from one place in code to another
  // in_bb and out_bb can be the same or a different bb
  static ins_iterator_t ins_move(BasicBlock &in_bb, ins_iterator_t in_beg,
                                 ins_iterator_t in_end, BasicBlock &out_bb,
                                 ins_iterator_t out_beg);

  std::string to_arg() const override;

  void dump(std::ostream &os) const override;

  BasicBlock(Function &fun, const std::string &name);

private:
  Function &_fun;
  PtrList<Instruction> _ins;

  friend class Instruction;
  friend class Function;
};

// IR Function, use basic block syntax
// The entry point is the first instruction in mod
class Function : public PtrListNode<Function>, public Value {

public:
  using decl_t = std::vector<std::string>;

  Function(const Function &) = delete;
  Function &operator=(const Function &) = delete;
  ~Function();

  Module &parent() { return _mod; }
  const Module &parent() const { return _mod; }

  // Return function type declaration
  const decl_t &decl() const { return _decl; }

  std::size_t args_count() const;

  ValueArg &get_arg(std::size_t pos) const;

  bool has_def() const { return !_no_def; }

  // Check if function is valid:
  // - all bbs must be valid
  // - the function must have an entry bb, which in the first in order
  void check() const;

  // Ordered list of Basic blocks
  bb_iterator_t bb_begin() { return _bbs.begin(); }
  const_bb_iterator_t bb_begin() const { return _bbs.begin(); }
  bb_iterator_t bb_end() { return _bbs.end(); }
  const_bb_iterator_t bb_end() const { return _bbs.end(); }
  IteratorRange<bb_iterator_t> bb() {
    return IteratorRange<bb_iterator_t>(bb_begin(), bb_end());
  }
  IteratorRange<const_bb_iterator_t> bb() const {
    return IteratorRange<const_bb_iterator_t>(bb_begin(), bb_end());
  }

  bool has_entry_bb() const { return _bb_entry != nullptr; }

  BasicBlock &get_entry_bb() {
    assert(_bb_entry);
    return *_bb_entry;
  }

  const BasicBlock &get_entry_bb() const {
    assert(_bb_entry);
    return *_bb_entry;
  }

  void set_entry_bb(BasicBlock &bb) {
    assert(&bb._fun == this);
    _bb_entry = &bb;
  }

  // Insert a basick blos at pos `it`
  // return iter to new bb
  bb_iterator_t insert_bb(bb_iterator_t it, const std::string &name = {});

  // Insert a basick block at the end
  BasicBlock &add_bb(const std::string &name = {});

  // Completely erase a basick block and its contentent from the function
  // Doesn't check if still referenced by some instructions
  void erase_bb(BasicBlock &bb);

  // Change basic block order
  // This is the order in which basic blocks are layed out in code
  // new_order must contain all basic blocks, and no duplicates
  void bb_order_change(const std::vector<BasicBlock *> &new_order) {
    _bbs.reorder(new_order);
  }

  std::string to_arg() const override;

  void dump(std::ostream &os) const override;

  Function(Module &mod, const std::string &name, const decl_t &decl,
           bool no_def);

private:
  Module &_mod;
  const decl_t _decl;
  std::vector<ValueOwner> _args;
  bool _no_def;
  NamesTable _ntable_bb;
  NamesTable _ntable_ins;

  PtrList<BasicBlock> _bbs;
  BasicBlock *_bb_entry;

  friend class Instruction;
  friend class BasicBlock;
  friend class Module;
};

// IR Module
// group multiple functions together
class Module {

public:
  Module(const Module &) = delete;
  Module &operator=(const Module &) = delete;
  ~Module();

  // List of all module functions
  fun_iterator_t fun_begin() { return _funs.begin(); }
  const_fun_iterator_t fun_begin() const { return _funs.begin(); }
  fun_iterator_t fun_end() { return _funs.end(); }
  const_fun_iterator_t fun_end() const { return _funs.end(); }
  IteratorRange<fun_iterator_t> fun() {
    return IteratorRange<fun_iterator_t>(fun_begin(), fun_end());
  }
  IteratorRange<const_fun_iterator_t> fun() const {
    return IteratorRange<const_fun_iterator_t>(fun_begin(), fun_end());
  }
  PtrList<Function> &fun_list() { return _funs; }
  const PtrList<Function> &fun_list() const { return _funs; }

  // Create an empty function without basic blocks
  Function &add_fun(const std::string &name,
                    const std::vector<std::string> &args, bool no_def = false);

  // Check if module is valid:
  // - All functions must be valid
  void check() const;

  // Create an empty module without any function
  static std::unique_ptr<Module> create();

private:
  PtrList<Function> _funs;
  NamesTable _ntable_fun;

  friend class Function;

  Module();
};
