#include "dep.hh"

#include <algorithm>
#include <cassert>

#include "ebb-view.hh"

namespace {

constexpr std::size_t INS_NONE = -1;

std::string get_label(const isa::Ins &ins) {
  if (ins.args().size() == 1)
    return ins.args()[0];

  return ins.args()[0] + ":" + ins.args()[1].substr(1);
}

std::size_t find_def_of(const EbbView &ebb, const std::string &reg,
                        std::size_t ins_idx) {
  std::size_t first_idx = 0;
  for (std::size_t i = ins_idx - 1; i < ins_idx && i >= first_idx; --i) {
    isa::Ins ins(ebb.ctx(), ebb[i]);
    auto defs = ins.args_defs();
    if (defs.size() == 1 && defs[0] == reg)
      return i;
  }
  return INS_NONE;
}

bool ins_use_reg(const EbbView &ebb, const std::string &reg,
                 std::size_t ins_idx) {
  isa::Ins ins(ebb.ctx(), ebb[ins_idx]);
  auto uses = ins.args_uses();
  return std::find(uses.begin(), uses.end(), reg) != uses.end();
}

} // namespace

Digraph make_dep_graph(const EbPaths::path_t &path) {
  EbbView ebb(path);
  Digraph g(ebb.size());

  for (std::size_t ins_idx = 0; ins_idx < ebb.size(); ++ins_idx) {
    isa::Ins ins(ebb.ctx(), ebb[ins_idx]);
    g.labels_set_vertex_name(ins_idx, get_label(ins));

    // Some searches for dependencies can start at bb begin
    // Because of the dependency between prev bb term and other ins
    std::size_t bb_beg_idx = ebb.offset_of(ebb.bb_of(ins_idx));

    for (auto r : ins.args_uses()) {
      // Find def of all uses
      auto def = find_def_of(ebb, r, ins_idx);
      if (def != INS_NONE) {
        assert(def < ins_idx);
        g.add_edge(def, ins_idx);
      }
    }

    // x before y and y def is used in x (antidependant)
    auto defs = ins.args_defs();
    if (defs.size() == 1)
      for (std::size_t prev = 0; prev < ins_idx; ++prev)
        if (ins_use_reg(ebb, defs.front(), prev))
          g.add_edge(prev, ins_idx);

    // find store before a load
    if (ins.opname() == "load")
      for (std::size_t prev = bb_beg_idx; prev < ins_idx; ++prev)
        if (ebb[prev][0] == "store")
          g.add_edge(prev, ins_idx);

    // find any before last terminal
    if (ins_idx + 1 == ebb.size())
      for (std::size_t prev = 0; prev < ins_idx; ++prev)
        if (g.succs_count(prev) == 0)
          g.add_edge(prev, ins_idx);

    // if ins has no preds, attach it to prev terminal bb
    // (also check if not first bb)
    if (bb_beg_idx && g.preds_count(ins_idx) == 0)
      g.add_edge(bb_beg_idx - 1, ins_idx);
  }

  return g;
}
