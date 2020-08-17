#pragma once

#include "../isa/module.hh"
#include "../utils/digraph.hh"
#include "eb-paths.hh"

// Build dependency graph
// Each node is an index in the instructions list of path
// Node x -> y means instruction y depends on instruction x
//  (x must be completed before y starts)
// x -> y if :
//  1) y uses reg defined in x
//  2) y is a load and x is a store
//  3) x before y and y def is used in x (antidependant)
//  4) y is a terminal in last bb and x is anything
//     (actually only add if x has no successors to simplify graph)
//  5) y is a root (no preds) and x the terminal in preceding bb
//     (this used to prevent an instruction to move to an earlier basic block
//     this is more complicated to fix that moving an instruction to a later bb
//     so it's disabled)
//
//  because of 5), there can't be no dependency that cross bb boundaries
//  at the exception of deps created by rules 4)
//  without this exception, all deps to last terminal would have to be local bb
//  term which would have resulted into no code motion at all allowed between
//  blocks
Digraph make_dep_graph(const EbPaths::path_t &path);
