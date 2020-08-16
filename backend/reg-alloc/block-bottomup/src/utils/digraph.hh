
#pragma once

#include "iterators.hh"
#include <cassert>
#include <logia/fwd.hh>
#include <ostream>
#include <vector>

class Digraph {

public:
  class succs_iter_t {

  public:
    succs_iter_t operator++() {
      _next();
      return *this;
    }

    succs_iter_t operator++(int) {
      auto res = *this;
      _next();
      return res;
    }

    std::size_t operator*() const {
      assert(_succ < _g._v);
      return _succ;
    }

  private:
    const Digraph &_g;
    std::size_t _v;
    std::size_t _succ;

    succs_iter_t(const Digraph &g, std::size_t v, std::size_t succ)
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

    friend bool operator==(const succs_iter_t &x, const succs_iter_t &y) {
      assert(&x._g == &y._g);
      assert(x._v == y._v);
      return x._succ == y._succ;
    }

    friend bool operator!=(const succs_iter_t &x, const succs_iter_t &y) {
      return !(x == y);
    }

    friend class Digraph;
  };

  class preds_iter_t {

  public:
    preds_iter_t operator++() {
      _next();
      return *this;
    }

    preds_iter_t operator++(int) {
      auto res = *this;
      _next();
      return res;
    }

    std::size_t operator*() const {
      assert(_pred < _g._v);
      return _pred;
    }

  private:
    const Digraph &_g;
    std::size_t _v;
    std::size_t _pred;

    preds_iter_t(const Digraph &g, std::size_t v, std::size_t pred)
        : _g(g), _v(v), _pred(pred) {
      if (pred == std::size_t(-1))
        _next();
    }

    void _next() {
      assert(_pred != _g._v);
      ++_pred;
      while (_pred < _g._v && !_g.has_edge(_pred, _v))
        ++_pred;
    }

    friend bool operator==(const preds_iter_t &x, const preds_iter_t &y) {
      assert(&x._g == &y._g);
      assert(x._v == y._v);
      return x._pred == y._pred;
    }

    friend bool operator!=(const preds_iter_t &x, const preds_iter_t &y) {
      return !(x == y);
    }

    friend class Digraph;
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

  void del_edge(std::size_t u, std::size_t v) {
    auto &edge = _adj[_mid(u, v)];
    _e -= edge;
    edge = 0;
  }

  bool has_edge(std::size_t u, std::size_t v) const {
    return _adj[_mid(u, v)] == 1;
  }

  // Return iterator over alls successors of u
  succs_iter_t succs_begin(std::size_t u) const {
    assert(u < _v);
    return succs_iter_t(*this, u, -1);
  }

  succs_iter_t succs_end(std::size_t u) const {
    assert(u < _v);
    return succs_iter_t(*this, u, _v);
  }

  std::size_t succs_count(std::size_t u) const;

  IteratorRange<succs_iter_t> succs(std::size_t u) const {
    return IteratorRange<succs_iter_t>(succs_begin(u), succs_end(u));
  }

  // Return iterator over all predecessors of u
  preds_iter_t preds_begin(std::size_t u) const {
    assert(u < _v);
    return preds_iter_t(*this, u, -1);
  }

  preds_iter_t preds_end(std::size_t u) const {
    assert(u < _v);
    return preds_iter_t(*this, u, _v);
  }

  IteratorRange<preds_iter_t> preds(std::size_t u) const {
    return IteratorRange<preds_iter_t>(preds_begin(u), preds_end(u));
  }

  std::size_t preds_count(std::size_t u) const;

  // Build a new graph with all edges reversed
  Digraph reverse() const;

  // Number of successors of u
  std::size_t out_deg(std::size_t u) const;

  // Number of predecessors of u
  std::size_t in_deg(std::size_t u) const;

  // dump to tree-file syntax
  void dump_tree(std::ostream &os) const;
  void dump_tree(logia::MdGfmDoc &doc) const;

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
