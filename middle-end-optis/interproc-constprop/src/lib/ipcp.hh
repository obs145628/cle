#pragma once

#include "module.hh"

// Use the SSA form to find function params that have constant values
// Use the call graph to perform constant progation through call sites
// Start with an optimistic value of any for every argument,
// then evaluate the value at each call site,
// use this info to update the corresponding formal argument value
// Then propagate this info to all call sites using this formal argument
// Keep going until reaching a fixed point solution.
// Most important is the code that evaluate an arg value at a call site,
// given the value of the formal arguments
// There are several ways to do this, from static eval of constants / formal
// arguments only, to eval simple computation tree, up to an instante of
// constant propagation algorithm.
//
// Algorithm Interprocedular Constant Propagation - Engineer a Compiler p522
void ipcp_run(Module &mod);
