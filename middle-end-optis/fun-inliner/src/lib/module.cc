#include "module.hh"

#include <cassert>
#include <set>

#include <iostream>

#include "ptr_list.hh"

#include "isa.hh"
#include <utils/cli/err.hh>

void Instruction::erase_from_parent() {
  parent().erase_ins(ins_iterator_t(this));
}

void Instruction::dump(std::ostream &os) const { isa::dump(*this, os); }

void BasicBlock::check() const {
  for (auto it = ins_begin(); it != ins_end(); ++it) {
    const auto &ins = *it;
    bool is_last = it + 1 == ins_end();

    PANIC_IF(&ins.parent() != this, "invalid instruction parent");

    PANIC_IF(isa::is_branch(ins) && !is_last,
             "branch instruction forbidden in the middle of a basicblock");
    PANIC_IF(!isa::is_branch(ins) && is_last,
             "last instruction must must be a branch");

    if (isa::is_branch(ins)) {
      for (const auto &t : isa::branch_targets(ins))
        PANIC_IF(!_fun.get_bb(t), "branch to invalid basick block");
    }
  }
}

void BasicBlock::erase_from_parent() { parent().erase_bb(*this); }

ins_iterator_t BasicBlock::ins_move(BasicBlock &in_bb, ins_iterator_t in_beg,
                                    ins_iterator_t in_end, BasicBlock &out_bb,
                                    ins_iterator_t out_beg) {
  if (in_beg == in_end) // empty range
    return out_beg;
  bool same_block = &in_bb == &out_bb;
  assert(!same_block); //@TODO handle same block

  auto res = in_bb._ins.move(in_beg, in_end, out_bb._ins, out_beg);

  if (!same_block) {
    for (auto it = in_beg; it != out_beg; ++it)
      it->_parent = &out_bb;
  }

  return res;
}

Function::Function(Module &mod, const std::string &name,
                   const std::vector<std::string> &args)
    : _mod(mod), _name(name), _args(args), _bb_entry(nullptr),
      _bb_unique_count(0) {}

void Function::check() const {
  PANIC_IF(_bb_entry == nullptr, "Entry BB not set");
  PANIC_IF(_bbs.index_of(const_bb_iterator_t(_bb_entry)) != 0,
           "Entry BB must be the first");
  for (const auto &bb : _bbs)
    bb.check();
}

BasicBlock *Function::get_bb(const std::string &label) {
  auto it = _bbs_labelsm.find(label);
  return it == _bbs_labelsm.end() ? nullptr : it->second;
}

const BasicBlock *Function::get_bb(const std::string &label) const {
  auto it = _bbs_labelsm.find(label);
  return it == _bbs_labelsm.end() ? nullptr : it->second;
}

bb_iterator_t Function::insert_bb(bb_iterator_t it, const std::string &label) {
  ++_bb_unique_count;
  std::string cpy_label =
      label.empty() ? ".L" + std::to_string(_bb_unique_count) : label;

  auto res = _bbs.emplace(it, *this, cpy_label);
  _bbs_labelsm.emplace(cpy_label, &*res);

  return res;
}

BasicBlock &Function::add_bb(const std::string &label) {
  return *insert_bb(bb_end(), label);
}

void Function::erase_bb(BasicBlock &bb) {
  _bbs_labelsm.erase(bb.label());
  if (&bb == _bb_entry)
    _bb_entry = nullptr;

  _bbs.erase(bb_iterator_t(&bb));
}

Function *Module::get_fun(const std::string &name) {
  auto it = _funs_map.find(name);
  return it == _funs_map.end() ? nullptr : it->second;
}

const Function *Module::get_fun(const std::string &name) const {
  auto it = _funs_map.find(name);
  return it == _funs_map.end() ? nullptr : it->second;
}

Function &Module::add_fun(const std::string &name,
                          const std::vector<std::string> &args) {
  PANIC_IF(get_fun(name) != nullptr, "Function name taken");

  auto &fun = *_funs.emplace(_funs.end(), *this, name, args);
  _funs_map.emplace(name, &fun);
  return fun;
}

void Module::check() const {
  for (const auto &f : _funs)
    f.check();
}

std::unique_ptr<Module> Module::create() {
  return std::unique_ptr<Module>(new Module{});
}

Module::Module() {}
