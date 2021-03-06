#include "cfg.hh"

#include <fstream>

Digraph build_cfg(const IModule &mod) {
  Digraph g(mod.bb_count());

  for (auto bb : mod.bb_list()) {
    g.labels_set_vertex_name(bb->id(), bb->label());

    const auto &bins = bb->ins().back();
    const auto &op = bins.args[0];

    if (op == "b") {
      g.add_edge(bb->id(), mod.get_bb(bins.args[1].substr(1))->id());
    }

    else if (op == "cbr") {
      g.add_edge(bb->id(), mod.get_bb(bins.args[2].substr(1))->id());
      g.add_edge(bb->id(), mod.get_bb(bins.args[3].substr(1))->id());
    }
  }

  std::ofstream ofs("cfg.dot");
  g.dump_tree(ofs);

  return g;
}
