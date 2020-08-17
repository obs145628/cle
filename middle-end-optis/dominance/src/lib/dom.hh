#pragma once

#include <llvm/IR/Module.h>

// Dominance
// Compute the dominance set of every basick block of a function
// block b_j in Dom(b_i) if all paths from b_0 to b_i contains b_j
// b_i in Dom(b_i)
// b_0 entry block
//
// Algorithm Dominance - Engineer a Compiler p478
void dom_run(const llvm::Module &mod);
