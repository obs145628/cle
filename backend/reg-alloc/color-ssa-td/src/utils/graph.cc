#include "graph.hh"

#include <fstream>

#include <mdlogger/mddocument.hh>

Graph::Graph(std::size_t v) : _v(v), _e(0), _adj(v * v, 0) {
  for (std::size_t i = 0; i < v; ++i)
    _labels_vs.push_back(".V" + std::to_string(i));
}

void Graph::dump_tree(std::ostream &os) const {
  os << "graph G{\n";
  for (std::size_t i = 0; i < _v; ++i)
    os << "  " << i << " [ label=\"" << _labels_vs[i] << "\" ];\n";

  for (std::size_t u = 0; u < _v; ++u)
    for (std::size_t v = u; v < _v; ++v)
      if (has_edge(u, v))
        os << "  " << u << " -- " << v << "\n";

  os << "}\n";
}

void Graph::dump_tree(MDDocument &doc) const {
  auto fname = doc.gen_file_name();
  std::ofstream os(doc.out_dir() + "/" + fname + ".dot");
  dump_tree(os);
  doc.image("graph", fname + ".png");
}

std::size_t Graph::degree(std::size_t u) const {
  std::size_t res = 0;
  for (std::size_t v = 0; v < _v; ++v)
    res += has_edge(u, v);
  return res;
}

void Graph::labels_set_vertex_name(std::size_t u, const std::string &name) {
  assert(u < _v);
  _labels_vs[u] = name;
}
