#include "bb.hh"

#include <utils/cli/err.hh>

namespace {

bool is_branch(const Ins &ins) {
  const auto &opname = ins.args[0];
  return opname == "b" || opname == "bgt" || opname == "ret";
}

} // namespace

BBList::BBList(const Module &mod) {
  BB curr;
  curr.idx = 0;
  curr.ins_beg = 0;

  // Make list of all bbs (divide by branch instructions)
  for (std::size_t pos = 0; pos < mod.code.size(); ++pos) {
    const auto &ins = mod.code[pos];

    if (is_branch(ins)) {
      curr.ins_end = pos + 1;
      _bbs.push_back(curr);
      _begs_map.emplace(curr.ins_beg, curr.idx);

      ++curr.idx;
      curr.ins_beg = pos + 1;
    }
  }
  PANIC_IF(curr.ins_beg != mod.code.size(),
           "Last instruction in module isn't a branch");

  // Make sure all branching is done to a basic block
  for (const auto &ins : mod.code) {
    const auto &op = ins.args[0];
    if (op == "b")
      _check_label(mod, ins.args[1]);
    else if (op == "bgt") {
      _check_label(mod, ins.args[3]);
      _check_label(mod, ins.args[4]);
    }
  }
}

void BBList::_check_label(const Module &mod, const std::string &lbl) {
  PANIC_IF(lbl.empty() || lbl[0] != '@', "Label must start with @");

  auto it = mod.labels.find(lbl.substr(1));
  PANIC_IF(it == mod.labels.end(), "Unknown label");

  auto bb_it = _begs_map.find(it->second);
  PANIC_IF(bb_it == _begs_map.end(),
           "Cannot branch to an instruction that doesn't start a basic block");
}
