#pragma once

#include "module.hh"

// merge 2 or more BBs into one
// A-B can be merged when A only branch to B, and B only pred is A
// Perform cleanup after inlining
void bbmerge(Function &fun);
