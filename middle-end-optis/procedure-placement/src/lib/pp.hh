#pragma once

#include "module-load.hh"
#include "module.hh"

// Procedure Placement
// Reorder the procedures in the module
// Try to put a caller function close to the callees
// The goal is to increase cache performance
// If P calls Q, and P and Q are next to each other and fit in the cache, it may
// prevent a cache miss.
// This is similar to Global Code Placement
// It may be impossible to put calless next to its callers in some cases.
// The algorithm use some heuristics to try and find a good (not optimal
// solution)
//
// // Algorithm Procedure Placement - Engineer a Compiler p462
void pp_run(Module &mod, const std::vector<CallInfos> &cg_freqs);
