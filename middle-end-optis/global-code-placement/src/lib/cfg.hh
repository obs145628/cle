#pragma once

#include "digraph.hh"
#include "imodule.hh"

// Build the Control Flow Graph
// G has n vertices, one for each basic block
// There is an edge from u to v if last instruction of bb u may branch to bb v
// The branch instructions must be annotated with how many times a branch was
// taken in comment
// These are given as weight to the CFG edges
Digraph build_cfg(const IModule &mod);
