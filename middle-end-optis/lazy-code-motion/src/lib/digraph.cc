#include "digraph.hh"

Digraph::Digraph(std::size_t v) : _v(v), _e(0), _adj(v * v, 0) {
  for (std::size_t i = 0; i < v; ++i)
    _labels_vs.push_back(".V" + std::to_string(i));
}

Digraph Digraph::reverse() const {
  Digraph res(_v);
  for (std::size_t u = 0; u < _v; ++u)
    for (auto it = adj_begin(u); it != adj_end(u); ++it)
      res.add_edge(*it, u);
  return res;
}

void Digraph::dump_tree(std::ostream &os) const {
  os << "digraph G{\n";
  for (std::size_t i = 0; i < _v; ++i)
    os << "  " << i << " [ label=\"" << _labels_vs[i] << "\" ];\n";

  for (std::size_t u = 0; u < _v; ++u) {
    for (auto it = adj_begin(u); it != adj_end(u); ++it)
      os << "  " << u << " -> " << *it << "\n";
  }

  os << "}\n";
}

std::vector<std::size_t> Digraph::get_preds(std::size_t u) const {
  std::vector<std::size_t> res;
  for (std::size_t v = 0; v < _v; ++v)
    if (has_edge(v, u))
      res.push_back(v);
  return res;
}

std::vector<std::size_t> Digraph::get_succs(std::size_t u) const {
  std::vector<std::size_t> res;
  for (std::size_t v = 0; v < _v; ++v)
    if (has_edge(u, v))
      res.push_back(v);
  return res;
}
