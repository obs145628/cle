#pragma once

#include "module.hh"

// Extensions to LVN:
// - Keep track of reg containing constant values
// - Perform constant folding
// - Assign same value number to commumative operations
// - Apply transformations for basic algebric ids (x+0, x*1, x-x, etc)
//
// Algorithm Local Value Numbering - Engineer a Compiler p???
void run_lvn_ext(Module &mod);
