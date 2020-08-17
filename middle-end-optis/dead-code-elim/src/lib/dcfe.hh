#pragma once

#include <llvm/IR/Module.h>

// Dead Control Flow Elimination
// Remove instructions that has no side-effects
// Delete unreachable blocks
//
// Second: Remove useless control flow
// Apply transformations that remove useless branch and empty blocks
// Iteratively try to find some patterns and simply it
// Stop when cannot simplyfy anymore
// Eliminating Useless Control Flow - Engineer a Compiler p547
void dcfe_run(llvm::Module &mod);
