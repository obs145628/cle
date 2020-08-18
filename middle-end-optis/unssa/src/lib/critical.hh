#pragma once

#include "module.hh"

// Split all critical edges of every function of mod
// u->v is critical iff u has more than 1 successor and v has more than 1
// predecessor Some SSA algorithms require to work with code that has no
// critical edge
// A critical edge can be converted in 2 non-critical edges by inserting
// a new basic block that makes the link between u and v
void critical_split(Module &mod);
