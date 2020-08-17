#pragma once

#include "digraph.hh"

// Pre-order: visit vertex before it's successors
// Post-order: visit vertex after its successors
// Reverse post-order: reverse order of post-order
enum class DFSOrder {
  PRE,
  POST,
  REV_POST,
};

// Compute a DFS ordering of the vertices
// order order of visit of the vertices
// start - first vertex to be visited
// visit_unreachable - visit vertices unreachable from start
std::vector<std::size_t> digraph_dfs(const Digraph &g, DFSOrder order,
                                     std::size_t start = 0,
                                     bool visit_unreachable = true);
