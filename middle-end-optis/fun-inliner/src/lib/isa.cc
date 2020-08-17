#include "isa.hh"
#include "gop.hh"

#include <cassert>

namespace isa {

bool is_branch(const Instruction &ins) {
  const auto &opname = ins.args[0];
  return opname == "b" || opname == "bc" || opname == "ret";
}

std::vector<std::string> branch_targets(const Instruction &ins) {
  const auto &op = ins.args[0];
  if (op == "b")
    return {ins.args[1].substr(1)};
  else if (op == "bc")
    return {ins.args[2].substr(1), ins.args[3].substr(1)};
  else if (op == "ret")
    return {};
  else
    assert(0);
}

void dump(const Instruction &ins, std::ostream &os) {
  gop::Ins(ins.args).dump(os);
}

} // namespace isa
