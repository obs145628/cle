#pragma once

#include "module.hh"

// Dominator Based Value Numbering Technique
// The code is simplified compared to usual VN because of SSA
// (Values cannot be redefined)
// Another advantage of SSA is that when transforming a block, we can use infos
// from all preds up to the root in the dominator tree
// And not just infos of a superblock
// This is because before nodes in other blocks when branching could redefine
// values, which wouldn't be visible if not in DOM list
// This isn't possible anymore thanks to SSA
//
// Dominator-based Value Numbering - Engineer a Compiler p566
void dvnt_run(Module &mod);
