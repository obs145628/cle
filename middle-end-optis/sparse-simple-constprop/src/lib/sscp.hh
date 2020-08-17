#pragma once

#include "module.hh"

// Use the SSA form to perform constant propagation
// The Trivial idea of performing constant propagation with the computation tree
// doesn't always work, because loops with computations (cycles in CFG) is
// represented by cyclic computation graph with phi-nodes.
// These cyclic computations cannot be solved even if the result is a constant
// This algorithm use a different approach that start with an initial value of
// every node, and propagate the known values through the computation tree and
// the cycles
//
// Algorithm Sparse Simple Constant Propagation - Engineer a Compiler p515
void sscp_run(Module &mod);
