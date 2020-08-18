#pragma once

#include <llvm/IR/Module.h>

/// Perform tree-height balancing
/// Algorithm applied to each basic block independently
/// On each bb, try to find expressions that are a sequence of the same,
/// commutative and associative operator. eg: a+b+c+d+e
/// Usually computed as ((((a+b)+c)+d)+e)
/// Rebalance such operation trees: eg: (((a + b) + (c + d)) + e)
/// This reduce the dependence between instructions for pipeling.
///
/// Actually I am not sure if it's really usefull
/// CPU perform instruction scheduling
/// And this looks more a backend than a middle-end opti
///
/// Algorithm Tree Height Balancing - Engineer a Compiler p428
void thb_run(llvm::Module &mod);
