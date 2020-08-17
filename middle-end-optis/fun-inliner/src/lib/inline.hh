#pragma once

#include "module.hh"

// Inline a specific function call
// `call_site` must be a call ins assigned to a basick block
// Perform many transformations:
// - Copy all basic blocks of callee into caller
// - make all renaming of regs / bbs necessary
// - devide call site bb in 2 (before / after call)
// - replace call to callee by jump to cloned entry block
// - replace all ret in cloned bbs by jump to after call bb
void inline_call(Instruction &call_site);
