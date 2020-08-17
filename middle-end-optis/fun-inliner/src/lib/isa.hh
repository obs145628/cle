#pragma once

#include "module.hh"

namespace isa {

// local jump or ret
bool is_branch(const Instruction &ins);

// possible labels for branchs ins
std::vector<std::string> branch_targets(const Instruction &ins);

void dump(const Instruction &ins, std::ostream &os);

} // namespace isa
