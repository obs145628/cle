#include "module.hh"

#include <cassert>
#include <iostream>
#include <set>

#include "../isa/isa.hh"
#include "cfg.hh"
#include <utils/cli/err.hh>

void Instruction::erase_from_parent() {
  parent().erase_ins(ins_iterator_t(this));
}

void Instruction::check() const { isa::check_ins(sargs()); }

const std::string &Instruction::get_opname() const { return _opname; }

std::vector<std::string> Instruction::sargs() const {
  std::vector<std::string> res{_opname};
  for (auto op : ops()) {
    if (res.size() == _def_idx)
      res.push_back(to_arg());
    res.push_back(op->to_arg());
  }
  return res;
}

bool Instruction::is_branch() const { return _is_branch; }

std::vector<BasicBlock *> Instruction::branch_targets() {
  assert(is_branch());
  std::vector<BasicBlock *> res;
  for (auto i : _targets_idx) {
    auto ptr = dynamic_cast<BasicBlock *>(&op(i));
    assert(ptr);
    res.push_back(ptr);
  }
  return res;
}

std::vector<const BasicBlock *> Instruction::branch_targets() const {
  assert(is_branch());
  std::vector<const BasicBlock *> res;
  for (auto i : _targets_idx) {
    auto ptr = dynamic_cast<const BasicBlock *>(&op(i));
    assert(ptr);
    res.push_back(ptr);
  }
  return res;
}

bool Instruction::has_def() const { return _def_idx != isa::IDX_NO; }

std::string Instruction::to_arg() const { return '%' + get_name(); }

void Instruction::dump(std::ostream &os) const {
  os << "Instruction(";
  isa::dump(os, sargs());
  os << ")";
}

Instruction::Instruction(const std::string &opname,
                         const std::vector<Value *> &ops,
                         const std::string &name, std::size_t def_idx,
                         BasicBlock &parent)
    : Value(ops, parent.parent()._ntable_ins, name), _parent(&parent),
      _opname(opname), _def_idx(def_idx) {
  _build_isa_infos();
}

void Instruction::_build_isa_infos() {
  auto args = sargs();
  isa::check_ins(args);
  assert(_def_idx == isa::def_idx(args));
  _is_branch = isa::is_branch(args);
  if (_is_branch)
    _targets_idx = isa::branch_targets_idxs(args);

  // Fix target idx index list
  for (auto &i : _targets_idx) {
    if (_def_idx != isa::IDX_NO && i >= _def_idx)
      i -= 2;
    else
      i -= 1;
  }
}

BasicBlock::~BasicBlock() { _ins.clear(); }

void BasicBlock::check() const {
  for (auto it = ins_begin(); it != ins_end(); ++it) {
    const auto &ins = *it;
    ins.check();
    bool is_last = it + 1 == ins_end();

    PANIC_IF(&ins.parent() != this, "invalid instruction parent");

    PANIC_IF(ins.is_branch() && !is_last,
             "branch instruction forbidden in the middle of a basicblock");
    PANIC_IF(!ins.is_branch() && is_last, "last instruction must be a branch");

    if (ins.is_branch())
      for (auto t : ins.branch_targets())
        PANIC_IF(&t->parent() != &parent(),
                 "branch to basick block of another function");
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

std::string BasicBlock::to_arg() const { return "@" + get_name(); }

void BasicBlock::dump(std::ostream &os) const {
  os << "Block(" << get_name() << ")";
}

BasicBlock::BasicBlock(Function &fun, const std::string &name)
    : Value({}, fun._ntable_bb, name), _fun(fun) {}

Function::~Function() {
  _bbs.clear();
  _args.clear();
}

Function::Function(Module &mod, const std::string &name, const decl_t &decl,
                   bool no_def)
    : Value({}, mod._ntable_fun, name), _mod(mod), _decl(decl), _ntable_bb("b"),
      _ntable_ins("r"), _bb_entry(nullptr) {

  auto args = isa::fundecl_args(_decl);
  for (std::size_t i = 0; i < args.size(); ++i)
    _args.emplace_back(ValueArg::make(*this, i, _ntable_ins, args[i]));
  _no_def = no_def;
}

std::size_t Function::args_count() const { return _args.size(); }

ValueArg &Function::get_arg(std::size_t pos) const {
  assert(pos < _args.size());
  auto res = dynamic_cast<ValueArg *>(_args[pos].get());
  assert(res);
  return *res;
}

void Function::check() const {
  if (!has_def())
    return;

  // Check entry bb is 0
  PANIC_IF(_bb_entry == nullptr, "Entry BB not set");
  PANIC_IF(_bbs.index_of(const_bb_iterator_t(_bb_entry)) != 0,
           "Entry BB must be the first");

  // Check all bbs
  for (const auto &bb : _bbs)
    bb.check();

  // Check there aren't multiple definitions with the same name
  std::set<std::string> defs;
  for (const auto &bb : _bbs)
    for (const auto &ins : bb.ins())
      if (ins.has_def())
        PANIC_IF(!defs.insert(ins.get_name()).second,
                 "Multiple value def of `" + ins.get_name() + "'");

  // Check phis have correct labels
  CFG cfg(*this);

  for (const auto &bb : _bbs) {
    auto preds = cfg.preds(bb);
    std::set<const BasicBlock *> spreds(preds.begin(), preds.end());

    for (const auto &ins : bb.ins()) {
      if (ins.get_opname() != "phi")
        continue;

      std::set<const BasicBlock *> phi_preds;
      PANIC_IF(2 * preds.size() != ins.ops_count(),
               "Invalid number of arguments for phi");
      for (std::size_t i = 0; i < ins.ops_count(); i += 2) {
        auto pred = dynamic_cast<const BasicBlock *>(&ins.op(i));
        assert(pred);
        phi_preds.insert(pred);
      }

      PANIC_IF(phi_preds != spreds,
               "Phi block doesn't match block predecessors");
    }

    // @TODO: incomplete check
    // must use dom-tree to make sure all uses are valid
  }
}

bb_iterator_t Function::insert_bb(bb_iterator_t it, const std::string &name) {
  assert(has_def());
  return _bbs.emplace(it, *this, name);
}

BasicBlock &Function::add_bb(const std::string &name) {
  return *insert_bb(bb_end(), name);
}

void Function::erase_bb(BasicBlock &bb) {
  if (&bb == _bb_entry)
    _bb_entry = nullptr;
  _bbs.erase(bb_iterator_t(&bb));
}

std::string Function::to_arg() const { return "@" + get_name(); }

void Function::dump(std::ostream &os) const {
  os << "Function(" << get_name() << "): ";
  isa::dump(os, _decl);
}

Function &Module::add_fun(const std::string &name,
                          const std::vector<std::string> &args, bool no_def) {
  return *_funs.emplace(_funs.end(), *this, name, args, no_def);
}

Module::~Module() { _funs.clear(); }

void Module::check() const {
  for (const auto &f : _funs)
    f.check();
}

std::unique_ptr<Module> Module::create() {
  return std::unique_ptr<Module>(new Module{});
}

Module::Module() : _ntable_fun("f") {}
