
#pragma once

#include <ostream>
#include <vector>

#include "iterators.hh"

class MDDocument;

class Graph {

public:
  class neighbors_iter_t {

  public:
    neighbors_iter_t operator++() {
      _next();
      return *this;
    }

    neighbors_iter_t operator++(int) {
      auto res = *this;
      _next();
      return res;
    }

    std::size_t operator*() const {
      assert(_succ < _g._v);
      return _succ;
    }

  private:
    const Graph &_g;
    std::size_t _v;
    std::size_t _succ;

    neighbors_iter_t(const Graph &g, std::size_t v, std::size_t succ)
        : _g(g), _v(v), _succ(succ) {
      if (succ == std::size_t(-1))
        _next();
    }

    void _next() {
      assert(_succ != _g._v);
      ++_succ;
      while (_succ < _g._v && !_g.has_edge(_v, _succ))
        ++_succ;
    }

    friend bool operator==(const neighbors_iter_t &x,
                           const neighbors_iter_t &y) {
      assert(&x._g == &y._g);
      assert(x._v == y._v);
      return x._succ == y._succ;
    }

    friend bool operator!=(const neighbors_iter_t &x,
                           const neighbors_iter_t &y) {
      return !(x == y);
    }

    friend class Graph;
  };

  Graph(std::size_t v);

  // returns numbers of vertices
  std::size_t v() const { return _v; }

  // returns number of edges
  std::size_t e() const { return _e; }

  void add_edge(std::size_t u, std::size_t v) {
    auto &edge = _adj[_mid(u, v)];
    _e += !edge;
    edge = 1;
  }

  void del_edge(std::size_t u, std::size_t v) {
    auto &edge = _adj[_mid(u, v)];
    _e -= edge;
    edge = 0;
  }

  bool has_edge(std::size_t u, std::size_t v) const {
    return _adj[_mid(u, v)] == 1;
  }

  // Return iterator over all neighbors of u
  neighbors_iter_t neighs_begin(std::size_t u) const {
    assert(u < _v);
    return neighbors_iter_t(*this, u, -1);
  }

  neighbors_iter_t neighs_end(std::size_t u) const {
    assert(u < _v);
    return neighbors_iter_t(*this, u, _v);
  }

  IteratorRange<neighbors_iter_t> neighs(std::size_t u) const {
    return IteratorRange<neighbors_iter_t>(neighs_begin(u), neighs_end(u));
  }

  // Number of neighbors of u
  std::size_t degree(std::size_t u) const;

  // dump to tree-file syntax
  void dump_tree(std::ostream &os) const;
  void dump_tree(MDDocument &doc) const;

  void labels_set_vertex_name(std::size_t u, const std::string &name);

private:
  const std::size_t _v;
  std::size_t _e;
  std::vector<int> _adj;

  std::vector<std::string> _labels_vs;

  std::size_t _mid(std::size_t u, std::size_t v) const {
    assert(u < _v);
    assert(v < _v);
    if (u > v)
      std::swap(u, v);
    return u * _v + v;
  }
};
