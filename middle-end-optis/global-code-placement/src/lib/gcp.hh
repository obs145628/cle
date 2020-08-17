#pragma once

#include "imodule.hh"

// Global Code Placement
// This technique use runtime profilling to reorder BBs in a more efficient
// manner
//
// When branching from 1 BB to another, it's cheaper when there are layed out in
// code one after the other (no branching)
// For example, for conditional branching, the fall through branch if faster. So
// it could be interesting to chose the more likely outcome as the fall through.
// This can also be applied to unconditional branching, eg when merging back
// return or if/else. GCP try to reorder BBs in order to optimize branching
// time. It's based on profilling: It has the number of times each branch (edge
// in CFG) was taken for an execution.
// The information is stored in comment line of the branch in IR files.
// The algorithms used to reorder BBs are rather heuristic
//
// Algorithm Global Code Placement - Engineer a Compiler p451
void gcp_run(IModule &mod);
