#include "digraph.hh"

Digraph::Digraph(std::size_t v) : _v(v), _e(0), _adj(v * v, 0) {
  for (std::size_t i = 0; i < v; ++i)
    _labels_vs.push_back(".V" + std::to_string(i));
}

Digraph Digraph::reverse() const {
  Digraph res(_v);
  res._labels_vs = _labels_vs;

  for (std::size_t u = 0; u < _v; ++u)
    for (std::size_t v : succs(u))
      res.add_edge(v, u);
  return res;
}

void Digraph::dump_tree(std::ostream &os) const {
  os << "digraph G{\n";
  for (std::size_t i = 0; i < _v; ++i)
    os << "  " << i << " [ label=\"" << _labels_vs[i] << "\" ];\n";

  for (std::size_t u = 0; u < _v; ++u)
    for (std::size_t v : succs(u))
      os << "  " << u << " -> " << v << "\n";

  os << "}\n";
}

std::size_t Digraph::out_deg(std::size_t u) const {
  std::size_t res = 0;
  for (std::size_t v = 0; v < _v; ++v)
    res += has_edge(u, v);
  return res;
}

std::size_t Digraph::in_deg(std::size_t u) const {
  std::size_t res = 0;
  for (std::size_t v = 0; v < _v; ++v)
    res += has_edge(v, u);
  return res;
}

bool operator==(const Digraph &a, const Digraph &b) {
  return a._v == b._v && a._e == b._e && a._adj == b._adj;
}
