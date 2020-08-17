#include "cfg.hh"

#include <cstdlib>

#include <utils/str/str.hh>

namespace {

std::vector<int> parse_weighs(const Ins &ins) {
  auto list = utils::str::split(utils::str::trim(ins.comm_eol), ' ');
  std::vector<int> res;
  for (const auto &n : list)
    res.push_back(atoi(n.c_str()));
  return res;
}

} // namespace

Digraph build_cfg(const IModule &mod) {
  Digraph g(mod.bb_count());

  for (auto bb : mod.bb_list()) {
    g.labels_set_vertex_name(bb->id(), bb->label());

    const auto &bins = bb->ins().back();
    const auto &op = bins.args[0];

    if (op == "b") {
      auto wei = parse_weighs(bins);
      assert(wei.size() == 1);
      g.add_edge(bb->id(), mod.get_bb(bins.args[1].substr(1))->id(), wei[0]);
    } else if (op == "beq") {
      auto wei = parse_weighs(bins);
      assert(wei.size() == 2);
      g.add_edge(bb->id(), mod.get_bb(bins.args[3].substr(1))->id(), wei[0]);
      g.add_edge(bb->id(), mod.get_bb(bins.args[4].substr(1))->id(), wei[1]);
    } else if (op == "tern") {
      auto wei = parse_weighs(bins);
      assert(wei.size() == 3);
      g.add_edge(bb->id(), mod.get_bb(bins.args[1].substr(1))->id(), wei[0]);
      g.add_edge(bb->id(), mod.get_bb(bins.args[2].substr(1))->id(), wei[1]);
      g.add_edge(bb->id(), mod.get_bb(bins.args[3].substr(1))->id(), wei[2]);
    }
  }

  return g;
}
