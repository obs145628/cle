#pragma once

#include "module.hh"

// SuperLocal Value Numbering
// This is an extension to LVN, that applies to many blocks at the same time
//
// LVN can be applied to several BBs, called a path, if each bb on the path has
// for only predecessor the bb before it on the path.
// LVN is run on all paths reachable from the start BB
//
// To avoid running the whole LVN on every BB of each path every time, a
// ScopedMap is used to divide infos by BBs. After visiting a BB, a scope is pop
// to remove all infos found on this block, and still have infos about the prev
// BBS
//
// Algorithm Super Local Value Numbering - Engineer a Compiler p437
void run_slvn(Module &mod);
