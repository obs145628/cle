#pragma once

#include "module.hh"

// Analyse all call sites of every function to decide whether to inline it or
// not
//
// For functions with inlined call sites, it run another pass to merge basic
// blocks (because every inline created a jump to a new bb, thay can be fusioned
// with original caller bb)
//
// Algorithm Inline Subsitution - Engineer a Compiler p458
void run_inline_pass(Module &mod);
