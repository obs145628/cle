#pragma once

#include "module.hh"
#include <gop10/module.hh>

// Convert module to gop::Module, but convert transform instructions to movs
// Code must have no critical edges
//
// Every time there is a phi, one mov is inserted at the end of every pred block
// with the right value.
// There must be no critical edges so that the code is added to a BB that has
// only one succ.
// Removing critical edges ensure correctess, but add extra jumps
// that slow down program, not optimal.
//
// The inserted mov may be produce incorrect because, this is because all phi
// instructions are run in parallel at the beginning of a block, but movs are
// sequential. If the input of some phi is the output of another phi you may get
// invalid results.
// This is solved by ordering the movs in such a way that no value get
// overwritten before being used. This is done by performing topological sort on
// a graph of asignments.
// Sometimes no ordering can save the problem because assignments are all
// dependants. (eg: x <- y; y <- x)
// This is represented as a cycle on the graph. These cycles can be removed by
// performing copies on tmp registers.
//
// Algorithm Translation Out of SSA Form - Engineer a Compiler p510
gop::Module unssa(const Module &mod);
