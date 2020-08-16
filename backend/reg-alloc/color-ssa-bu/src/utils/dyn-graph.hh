#pragma once

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include <logia/fwd.hh>

// Dynamic undirect graph
// Can add / remove edges
// T is the vertex type
template <class T> class DynGraph {

public:
  DynGraph() = default;

  std::size_t v() const { return _edges.size(); }

  void add_edge(const T &u, const T &v) {
    _edges.at(u).insert(v);
    _edges.at(v).insert(u);
  }

  void del_edge(const T &u, const T &v) {
    _edges.at(u).erase(v);
    _edges.at(v).erase(u);
  }

  bool has_edge(const T &u, const T &v) const { return _edges.at(u).count(v); }

  void add_vertex(const T &u) {
    if (!_edges.count(u))
      _edges[u] = {};
  }

  void del_vertex(const T &u) {
    if (!_edges.count(u))
      return;

    _edges.erase(u);
    for (auto it : _edges)
      it.second.erase(u);
  }

  bool has_vertex(const T &u) const { return _edges.count(u); }

  const std::set<T> &neighs(const T &u) const { return _edges.at(u); }

  std::vector<T> vertices() const {
    std::vector<T> res;
    for (auto e : _edges)
      res.push_back(e.first);
    return res;
  }

  std::size_t degree(std::size_t u) const { return neighs(u).size(); }

  void dump_tree(std::ostream &os) const;
  void dump_tree(logia::MdGfmDoc &doc) const;

private:
  std::map<T, std::set<T>> _edges;
};

#include "dyn-graph.hxx"
