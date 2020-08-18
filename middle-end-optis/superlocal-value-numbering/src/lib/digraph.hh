
#pragma once

#include <cassert>
#include <ostream>
#include <vector>

class Digraph {

public:
  class adj_iter_t {

  public:
    adj_iter_t operator++() {
      _next();
      return *this;
    }

    adj_iter_t operator++(int) {
      auto res = *this;
      _next();
      return res;
    }

    std::size_t operator*() const {
      assert(_adj_i < _g._v);
      return _adj_i;
    }

  private:
    const Digraph &_g;
    std::size_t _v;
    std::size_t _adj_i;

    adj_iter_t(const Digraph &g, std::size_t v, std::size_t adj_i)
        : _g(g), _v(v), _adj_i(adj_i) {}

    void _next() {
      assert(_adj_i != _g._v);
      ++_adj_i;
      while (_adj_i < _g._v && !_g.has_edge(_v, _adj_i))
        ++_adj_i;
    }

    friend class Digraph;
    friend bool operator==(const adj_iter_t &x, const adj_iter_t &y);
  };

  Digraph(std::size_t v);

  // returns numbers of vertices
  std::size_t v() const { return _v; }

  // returns number of edges
  std::size_t e() const { return _e; }

  void add_edge(std::size_t u, std::size_t v) {
    auto &edge = _adj[_mid(u, v)];
    _e += !edge;
    edge = 1;
  }

  bool has_edge(std::size_t u, std::size_t v) const {
    return _adj[_mid(u, v)] == 1;
  }

  // Return iterator over all vertices adjacent to u
  adj_iter_t adj_begin(std::size_t u) const {
    assert(u < _v);
    adj_iter_t res(*this, u, -1);
    return ++res;
  }

  adj_iter_t adj_end(std::size_t u) const {
    assert(u < _v);
    return adj_iter_t(*this, u, _v);
  }

  // Build a new graph with all edges reversed
  Digraph reverse() const;

  // Number of successors of u
  std::size_t out_deg(std::size_t u) const;

  // dump to tree-file syntax
  void dump_tree(std::ostream &os) const;

  void labels_set_vertex_name(std::size_t u, const std::string &name) {
    assert(u < _v);
    _labels_vs[u] = name;
  }

private:
  const std::size_t _v;
  std::size_t _e;
  std::vector<int> _adj;

  std::vector<std::string> _labels_vs;

  std::size_t _mid(std::size_t u, std::size_t v) const {
    assert(u < _v);
    assert(v < _v);
    return u * _v + v;
  }
};

inline bool operator==(const Digraph::adj_iter_t &x,
                       const Digraph::adj_iter_t &y) {
  assert(&x._g == &y._g);
  assert(x._v == y._v);
  return x._adj_i == y._adj_i;
}

inline bool operator!=(const Digraph::adj_iter_t &x,
                       const Digraph::adj_iter_t &y) {
  return !(x == y);
}

inline std::size_t Digraph::out_deg(std::size_t u) const {
  std::size_t res = 0;
  for (auto it = adj_begin(u); it != adj_end(u); ++it)
    ++res;
  return res;
}
