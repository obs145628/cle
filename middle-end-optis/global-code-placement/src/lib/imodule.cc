#include "imodule.hh"

#include <cassert>
#include <set>

#include <utils/cli/err.hh>

namespace {

bool is_branch(const Ins &ins) {
  const auto &opname = ins.args[0];
  return opname == "b" || opname == "beq" || opname == "ret" ||
         opname == "tern";
}

} // namespace

void BasicBlock::check() const {
  for (std::size_t i = 0; i < _ins.size(); ++i) {
    const auto &ins = _ins[i];

    PANIC_IF(is_branch(ins) && i + 1 < _ins.size(),
             "branch instruction forbidden in the middle of a basicblock");
    PANIC_IF(!is_branch(ins) && i + 1 == _ins.size(),
             "last instruction must must be a branch");

    // @TODO Check if branch to a basicblock
    const auto &op = ins.args[0];
    if (op == "b")
      PANIC_IF(!_mod.get_bb(ins.args[1].substr(1)), "b: invalid label name");
    else if (op == "beq") {
      PANIC_IF(!_mod.get_bb(ins.args[3].substr(1)) ||
                   !_mod.get_bb(ins.args[4].substr(1)),
               "beq: invalid label name");
    }
  }
}

IModule::IModule() : _bb_entry(nullptr), _bb_next_id(0) {}

void IModule::check() const {
  PANIC_IF(_bb_entry == nullptr, "Entry BB not set");
  PANIC_IF(_bb_entry->_order != 0, "Entry BB must be the first");
  for (const auto &bb : _bbs_list)
    bb->check();
}

std::vector<BasicBlock *> IModule::bb_list() {
  std::vector<BasicBlock *> res;
  for (auto &bb : _bbs_list)
    res.push_back(bb.get());
  return res;
}

std::vector<const BasicBlock *> IModule::bb_list() const {
  std::vector<const BasicBlock *> res;
  for (const auto &bb : _bbs_list)
    res.push_back(bb.get());
  return res;
}

BasicBlock *IModule::get_bb(bb_id_t id) {
  auto it = _bbs_idsm.find(id);
  return it == _bbs_idsm.end() ? nullptr : it->second;
}

BasicBlock *IModule::get_bb(const std::string &label) {
  auto it = _bbs_labelsm.find(label);
  return it == _bbs_labelsm.end() ? nullptr : it->second;
}

const BasicBlock *IModule::get_bb(bb_id_t id) const {
  auto it = _bbs_idsm.find(id);
  return it == _bbs_idsm.end() ? nullptr : it->second;
}

const BasicBlock *IModule::get_bb(const std::string &label) const {
  auto it = _bbs_labelsm.find(label);
  return it == _bbs_labelsm.end() ? nullptr : it->second;
}

BasicBlock &IModule::add_bb(const std::string &label) {
  auto id = _bb_next_id++;
  std::string cpy_label = label.empty() ? ".L" + std::to_string(id) : label;

  _bbs_list.emplace_back(new BasicBlock(*this, id, cpy_label,
                                        std::vector<Ins>{}, _bbs_list.size()));
  BasicBlock *bb = _bbs_list.back().get();

  _bbs_idsm.emplace(id, bb);
  _bbs_labelsm.emplace(cpy_label, bb);
  return *bb;
}

void IModule::bb_order_change(
    const std::vector<const BasicBlock *> &new_order) {
  // Check if new_order contain all bbs, no more
  PANIC_IF(new_order.size() != _bbs_list.size(), "invalid bb list");
  std::set<const BasicBlock *> bbs;
  for (const auto &bb : _bbs_list)
    bbs.insert(bb.get());
  for (auto bb : new_order)
    bbs.erase(bb);
  PANIC_IF(!bbs.empty(), "invalid bb list");

  // reorder bbs
  for (std::size_t i = 0; i < new_order.size(); ++i)
    bb_order_swap(_bbs_list[i].get(), new_order[i]);
}

void IModule::bb_order_swap(const BasicBlock *b1, const BasicBlock *b2) {
  auto order1 = b1->_order;
  auto order2 = b2->_order;
  std::swap(_bbs_list[order1], _bbs_list[order2]);
  std::swap(b1->_order, b2->_order);
}

std::unique_ptr<IModule> mod2imod(const Module &mod) {
  auto res = std::make_unique<IModule>();
  assert(mod.code.size() > 0);

  BasicBlock *next_bb = nullptr;

  // Make list of all bbs (divide by branch instructions)
  for (auto ins : mod.code) {

    if (next_bb == nullptr) {
      auto label = ins.label_defs.size() > 0 ? ins.label_defs[0] : "";
      next_bb = &res->add_bb(label);
      if (!res->has_entry_bb())
        res->set_entry_bb(*next_bb);
    }

    ins.label_defs.clear();
    next_bb->ins().push_back(ins);

    if (is_branch(ins)) // end of basic block
      next_bb = nullptr;
  }

  // Check module is well formed
  PANIC_IF(next_bb != nullptr, "Last instruction in module isn't a branch");
  res->check();
  return res;
}

Module imod2mod(const IModule &mod) {
  Module res;
  for (const auto &bb : mod.bb_list()) {
    auto first =
        res.code.insert(res.code.end(), bb->ins().begin(), bb->ins().end());
    first->label_defs = {bb->label()};

    std::size_t first_idx = &*first - &res.code[0];
    res.labels.emplace(bb->label(), first_idx);
  }

  return res;
}
