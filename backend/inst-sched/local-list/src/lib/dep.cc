#include "dep.hh"

#include <cassert>

namespace {

constexpr std::size_t INS_NONE = -1;

std::string get_label(const isa::Ins &ins) {
  if (ins.args().size() == 1)
    return ins.args()[0];

  return ins.args()[0] + ":" + ins.args()[1].substr(1);
}

std::size_t find_def_of(const isa::BasicBlock &bb, const std::string &reg) {
  for (std::size_t i = 0; i < bb.code().size(); ++i) {
    isa::Ins ins(bb.parent().parent().ctx(), bb.code()[i]);
    auto defs = ins.args_defs();
    if (defs.size() == 1 && defs[0] == reg)
      return i;
  }
  return INS_NONE;
}

} // namespace

Digraph make_dep_graph(const isa::BasicBlock &bb) {
  Digraph g(bb.code().size());

  for (std::size_t i = 0; i < bb.code().size(); ++i) {
    isa::Ins ins(bb.parent().parent().ctx(), bb.code()[i]);
    g.labels_set_vertex_name(i, get_label(ins));

    for (auto r : ins.args_uses()) {
      // Find def of all uses
      auto def = find_def_of(bb, r);
      if (def != INS_NONE) {
        assert(def < i);
        g.add_edge(def, i);
      }
    }

    // find store before a load / ret
    if (ins.opname() == "load" || ins.opname() == "ret" ||
        ins.opname() == "retv")
      for (std::size_t prev = 0; prev < i; ++prev)
        if (bb.code()[prev][0] == "store")
          g.add_edge(prev, i);
  }

  return g;
}
