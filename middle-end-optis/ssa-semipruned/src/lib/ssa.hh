#pragma once

#include "module.hh"

// Transform a program into SSA-form
// This require multiples step:
// 1) build dom-tree
// 2) build dominance frontier
// 3) From dominance frontier, it's possible to locate possible positions where
//    a phi-node is needed
// 4) Some of these phi-nodes are not live, and other
//    techniques can be used to remove it using liveness analysis (full pruning)
// 5) Rename every def in the code to get unique names
//
// Algorithm SSA semi-pruned conversion - Engineer a Compiler p496
void ssa_run(Module &mod);
