#include "digraph.hh"

#include <fstream>

#include <mdlogger/mddocument.hh>

Digraph::Digraph(std::size_t v) : _v(v), _e(0), _adj(v * v, 0) {
  for (std::size_t i = 0; i < v; ++i)
    _labels_vs.push_back(".V" + std::to_string(i));
}

std::size_t Digraph::succs_count(std::size_t u) const {
  std::size_t res = 0;
  for (auto x : succs(u)) {
    (void)x;
    ++res;
  }
  return res;
}

std::size_t Digraph::preds_count(std::size_t u) const {
  std::size_t res = 0;
  for (auto x : preds(u)) {
    (void)x;
    ++res;
  }
  return res;
}

Digraph Digraph::reverse() const {
  Digraph res(_v);
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

void Digraph::dump_tree(MDDocument &doc) const {
  auto fname = doc.gen_file_name();
  std::ofstream os(doc.out_dir() + "/" + fname + ".dot");
  dump_tree(os);
  doc.image("digraph", fname + ".png");
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
