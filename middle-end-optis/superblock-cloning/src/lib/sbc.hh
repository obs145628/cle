#pragma once

#include "module.hh"

// Superblock Cloning
// Optimize a loop by cloning all blocks in every path from loop begin to loop
// end Every time a block jump to a next bb (1 succ only), This block is cloned
// instead This way, these jumps can be eliminated later by fusing the 2 blocks
// (Wasn't possible before because the bb that it jumps to had more than 1 pred,
// but thanks to the clone it only has one now)
// This help reduce branching, and optimize better code for the specialized
// version of the block.
// But it may also increase code size
//
// Algorithm Superblock Cloning - Engineer a Compiler p570
void sbc_run(Module &mod);
