#include "gcp.hh"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>

#include "cfg.hh"

namespace {

class GCP {

public:
  GCP(IModule &mod) : _mod(mod), _cfg(build_cfg(mod)) {}

  void run() {
    // Step 1 build CFG
    std::ofstream ofs("out.dot");
    _cfg.dump_tree(ofs);

    // Step 2 get edges sorted by decreasing order
    for (std::size_t u = 0; u < _cfg.v(); ++u)
      for (auto it = _cfg.adj_begin(u); it != _cfg.adj_end(u); ++it)
        _edges.push_back(*it);
    std::sort(_edges.begin(), _edges.end(),
              [this](const Digraph::edge_t &a, const Digraph::edge_t &b) {
                return a.weight > b.weight;
              });

    // Step 3 build hot paths
    _build_hots_paths();
    _dump_hots_paths();

    // Step 4 compute new bbs order
    _reorder_bbs();

    _mod.bb_order_change(_new_order);
    _mod.check();
  }

private:
  using chain_t = std::vector<const BasicBlock *>;

  IModule &_mod;
  Digraph _cfg;
  std::vector<Digraph::edge_t> _edges;
  std::vector<chain_t> _hots_paths;
  std::vector<std::size_t> _hot_paths_prio;
  std::vector<const BasicBlock *> _new_order;

  // return index of the chain where bb is
  std::size_t _find_chain(const BasicBlock *bb) {
    for (std::size_t i = 0; i < _hots_paths.size(); ++i)
      if (std::find(_hots_paths[i].begin(), _hots_paths[i].end(), bb) !=
          _hots_paths[i].end())
        return i;
    assert(0);
  }

  // Build hot paths
  // A path is just a sequence of BBs that can be followed by execution
  // Each path is assigned a priority: the lower, the more probable it is the
  // program will take that path
  // The number of chains and the calcul of priority is heuristic
  void _build_hots_paths() {
    // Initialize hot_paths with e chains of size 1, each containing 1 block
    // with max priority
    std::size_t max_p = _edges.size();
    for (auto bb : _mod.bb_list()) {
      _hots_paths.push_back({bb});
      _hot_paths_prio.push_back(max_p);
    }

    std::size_t p = 0;
    for (const auto &edge : _edges) {
      if (edge.v == edge.w) // ignore self-loops
        continue;

      auto bb_v = _mod.get_bb(edge.v);
      auto bb_w = _mod.get_bb(edge.w);
      auto chain_v = _find_chain(bb_v);
      auto chain_w = _find_chain(bb_w);
      if (_hots_paths[chain_v].back() != bb_v ||
          _hots_paths[chain_w].front() != bb_w) // cannot be merged
        continue;

      // prio[v] = min(p, prio[v], prio[w])
      std::size_t prio = std::min(
          p++, std::min(_hot_paths_prio[chain_v], _hot_paths_prio[chain_w]));
      _hot_paths_prio[chain_v] = prio;
      _hot_paths_prio.erase(_hot_paths_prio.begin() + chain_w);

      // chain[v] += chain[w]
      _hots_paths[chain_v].insert(_hots_paths[chain_v].end(),
                                  _hots_paths[chain_w].begin(),
                                  _hots_paths[chain_w].end());
      _hots_paths.erase(_hots_paths.begin() + chain_w);
    }
  }

  // Reorder basic blocks
  // Reorder them by putting all bbs of chain, then of another, etc
  // It starts with the chain that has the entry
  // Then put chains in order related to their priority
  // The chain order is heuristic
  void _reorder_bbs() {
    auto cmp_fn = [this](std::size_t c1, std::size_t c2) {
      return _hot_paths_prio[c1] > _hot_paths_prio[c2];
    };
    std::priority_queue<std::size_t, std::vector<std::size_t>, decltype(cmp_fn)>
        work(cmp_fn);

    std::set<std::size_t> visited; // chains already added to queue

    auto echain = _find_chain(&_mod.get_entry_bb());
    work.push(echain);
    visited.insert(echain);

    while (!work.empty()) {
      std::size_t cid = work.top();
      work.pop();
      const auto &chain = _hots_paths[cid];
      for (auto bb : chain)
        _new_order.push_back(bb);

      for (auto bb : chain)
        for (auto it = _cfg.adj_begin(bb->id()); it != _cfg.adj_end(bb->id());
             ++it) {
          auto next_bb = _mod.get_bb((*it).w);
          auto cnext = _find_chain(next_bb);
          if (visited.insert(cnext).second)
            work.push(cnext);
        }
    }
  }

  void _dump_chain(const chain_t &chain) {
    std::cout << "{";
    for (std::size_t i = 0; i < chain.size(); ++i) {
      std::cout << chain[i]->label();
      if (i + 1 < chain.size())
        std::cout << ", ";
    }
    std::cout << "}";
  }

  void _dump_hots_paths() {
    std::cout << "hots paths:\n";
    for (std::size_t i = 0; i < _hots_paths.size(); ++i) {
      _dump_chain(_hots_paths[i]);
      std::cout << "; P = " << _hot_paths_prio[i] << "\n";
    }
    std::cout << "\n";
  }
}; // namespace

} // namespace

void gcp_run(IModule &mod) {
  GCP gcp(mod);
  gcp.run();
}
