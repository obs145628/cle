#pragma once

#include "bb.hh"
#include "digraph.hh"
#include "module.hh"

// Build the Control Flow Graph
// G has n vertices, one for each basic block
// There is an edge from u to v if last instruction of bb u may branch to bb v
Digraph build_cfg(const Module &mod, const BBList &bbs);
