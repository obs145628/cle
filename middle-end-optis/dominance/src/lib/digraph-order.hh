#pragma once

#include "digraph.hh"

// Compute a DFS ordering of the vertices
// Pre-order: visit vertex before it's successors
// Post-order: visit vertex after its successors
// Reverse post-order: reverse order of post-order

std::vector<std::size_t> preorder(const Digraph &g);
std::vector<std::size_t> postorder(const Digraph &g);
std::vector<std::size_t> rev_postorder(const Digraph &g);
