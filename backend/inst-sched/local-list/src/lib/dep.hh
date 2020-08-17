#pragma once

#include "../isa/module.hh"
#include "../utils/digraph.hh"

// Build dependency graph
// Each node is an index in the instructions list of bb
// Node x -> y means instruction y depends on instruction x
//  (x must be completed before y starts)
// x -> y if :
//  - y uses reg defined in x
//  - y is a load and x is a store
//  - y is a ret and x is a store
//    (actually x->y true for any x if y is a ret, but makes ex simpler)
Digraph make_dep_graph(const isa::BasicBlock &bb);
