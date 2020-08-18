#include "cfg.hh"

#include <map>

namespace {

std::size_t label2bb(const Module &mod, const BBList &bbs,
                     const std::string &label) {
  auto it = mod.labels.find(label.substr(1));
  assert(it != mod.labels.end());
  return bbs.get_at(it->second).idx;
}

} // namespace

Digraph build_cfg(const Module &mod, const BBList &bbs) {
  Digraph g(bbs.count());

  for (const auto &bb : bbs.all()) {
    const auto &start = mod.code[bb.ins_beg];
    assert(start.label_defs.size() == 1);
    g.labels_set_vertex_name(bb.idx, start.label_defs[0]);

    const auto &bins = mod.code[bb.ins_end - 1];
    const auto &op = bins.args[0];

    if (op == "b") {
      g.add_edge(bb.idx, label2bb(mod, bbs, bins.args[1]));
    } else if (op == "bgt") {
      g.add_edge(bb.idx, label2bb(mod, bbs, bins.args[3]));
      g.add_edge(bb.idx, label2bb(mod, bbs, bins.args[4]));
    }
  }

  return g;
}
