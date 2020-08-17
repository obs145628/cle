#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "module.hh"

class IModule;

using bb_id_t = std::size_t;

class BasicBlock {

public:
  BasicBlock(const BasicBlock &) = delete;
  BasicBlock &operator=(const BasicBlock &) = delete;

  // Unique const identifier from 0 to #bbs
  bb_id_t id() const { return _id; }

  // Unique const string identifier
  const std::string &label() const { return _label; }

  std::vector<Ins> &ins() { return _ins; }
  const std::vector<Ins> &ins() const { return _ins; }

  // Check if a basic block is valid.
  // must respect all these rules:
  // - a bb must have at least one instruction
  // - a not-last instruction in a bb must not branch
  // - the last instruction in a bb must branch
  // - a branching must be to the beginning of a basic block
  void check() const;

private:
  const IModule &_mod;
  std::vector<Ins> _ins;
  const bb_id_t _id;
  const std::string _label;

  friend class IModule;

  BasicBlock(const IModule &mod, bb_id_t id, const std::string &label,
             const std::vector<Ins> &ins)
      : _mod(mod), _ins(ins), _id(id), _label(label) {}
};

// IR Module, use basic block syntax
// The entry point is the first instruction in mod
// Another empty entry pointer that jumps to this one is added
class IModule {

public:
  // Create an empty module without basic block
  IModule();

  IModule(const IModule &) = delete;
  IModule &operator=(const IModule &) = delete;

  // Check if module is valid:
  // - all bbs must be valid
  // - the module must have entry bb
  void check() const;

  std::size_t bb_count() const { return _bbs_list.size(); }

  // Ordered list of Basic blocks
  // @Extra use iterators would be a better design
  std::vector<BasicBlock *> bb_list();
  std::vector<const BasicBlock *> bb_list() const;

  // Find a basicblock with its index or label
  // Returns a nullptr if not found
  BasicBlock *get_bb(bb_id_t id);
  BasicBlock *get_bb(const std::string &label);
  const BasicBlock *get_bb(bb_id_t id) const;
  const BasicBlock *get_bb(const std::string &label) const;

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
    assert(&bb._mod == this);
    _bb_entry = &bb;
  }

  BasicBlock &add_bb(const std::string &label = {});

private:
  std::vector<std::unique_ptr<BasicBlock>> _bbs_list;
  BasicBlock *_bb_entry;
  bb_id_t _bb_next_id;
  std::map<bb_id_t, BasicBlock *> _bbs_idsm;
  std::map<std::string, BasicBlock *> _bbs_labelsm;
};

// Build a IModule given classic module
// Divide body into sequence of basic blocks
// Check if all basic blocks are well formed
// Also add a special bb _entry with one ins that jump to first bb
std::unique_ptr<IModule> mod2imod(const Module &mod);

// Build a Module from a IModule
// Instructions are put in the order of the basic blocks
// Labels are basic block labels
Module imod2mod(const IModule &mod);
