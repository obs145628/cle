#pragma once

#include "dyn-graph.hh"

#include <fstream>
#include <vector>

#include <logia/md-gfm-doc.hh>

template <class T> inline void DynGraph<T>::dump_tree(std::ostream &os) const {
  os << "graph G {\n";
  std::vector<T> vs = vertices();

  for (std::size_t i = 0; i < vs.size(); ++i)
    os << "  " << i << " [ label=\"" << vs[i] << "\" ];\n";

  for (std::size_t u = 0; u < vs.size(); ++u)
    for (std::size_t v = u; v < vs.size(); ++v)
      if (has_edge(vs[u], vs[v]))
        os << "  " << u << " -- " << v << "\n";

  os << "}\n";
}

template <class T>
inline void DynGraph<T>::dump_tree(logia::MdGfmDoc &doc) const {
  auto fname = doc.gen_file_name();
  std::ofstream os(doc.out_dir() + "/" + fname + ".dot");
  dump_tree(os);
  doc.image("graph", doc.get_file_path(fname + ".png"));
}
