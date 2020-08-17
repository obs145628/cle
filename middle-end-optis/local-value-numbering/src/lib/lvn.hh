#pragma once

#include "module.hh"

// Apply Local Value Numbering
// It can be applied on every basic block of a function independently
//
// In these examples, the module represent a single basic block
// It works by assigning a unique value to each different expression
// If we find an already known value, it means the expression was already
// computed earlier We can then replace the whole expression by a simply copy of
// the old result
//
// Algorithm Local Value Numbering - Engineer a Compiler p420
void run_lvn(Module &mod);
