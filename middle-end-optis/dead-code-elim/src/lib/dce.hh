#pragma once

#include <llvm/IR/Module.h>

// Dead Code Elimination
// Remove instructions that has no side-effects
// Delete unreachable blocks
//
// First: Remove useless instructions
// Mark and sweep
// Mark pass: go through all instructions and mark all critical instructions
// (side effects)
// Recursively mark instructions that def values used by other marked
// instructions
// Sweep pass : remove all unmarked instructions Algorithm
// Eliminating Ueseless Code - Engineer a Compiler p544
//
// Second: Remove useless control flow
void dce_run(llvm::Module &mod);
