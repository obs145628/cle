#pragma once

#include <llvm/IR/Module.h>

// Dead Control Flow Elimination
// Remove instructions that has no side-effects
// Delete unreachable blocks
//
// Third: Remove unreachable blocks
// Apply transformations that remove useless branch and empty blocks
// Iteratively try to find some patterns and simply it
// Stop when cannot simplyfy anymore
// Eliminating Unreachable code - Engineer a Compiler p550
void unreachable_run(llvm::Module &mod);
