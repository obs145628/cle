#pragma once

#include "module.hh"

// Sparse Conditional Constant Propagation
// Perform Constrant Propagation using both SSA-graph and CFG
// Like Simple constant, use latice values and optimistic initialization to be
// able to propagate constants through cycles.
// Contrary to simple version, only evaluate constants using infos from the CFG
// edges that get executed.
// Unreachable code is ignored
// This can find more constants (eg for phi, only consider the value for the
// edges that get taken
//
// Algorithm Sparse Conditional Constant Propagation  - Engineer a Compiler p575
// Paper Constant Propagation with Conditional Branches
void scc_run(Module &mod);
