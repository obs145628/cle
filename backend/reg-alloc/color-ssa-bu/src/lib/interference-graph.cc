#include "interference-graph.hh"

#include "../isa/isa.hh"
#include "live-now.hh"
#include <logia/program.hh>

InterferenceGraph::InterferenceGraph(const isa::Function &fun)
    : isa::FunctionAnalysis(fun) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(
      "Interference Graph: @" + fun.name());
  _build();
  _doc = nullptr;
}

void InterferenceGraph::_build() {
  // Build the interference graph fro the Live Ranges
  // Two live ranges LR_i and LR_j interferes if one is live at the definition
  // of the other

  // Count number of live ranges
  std::set<std::string> lrs;
  for (auto bb : fun().bbs())
    for (const auto &ins : bb->code()) {
      isa::Ins cins(fun().parent().ctx(), ins);
      for (const auto &r : cins.args_uses())
        if (r.size() > 2 && r[0] == 'l' && r[1] == 'r')
          lrs.insert(r);
      for (const auto &r : cins.args_defs())
        if (r.size() > 2 && r[0] == 'l' && r[1] == 'r')
          lrs.insert(r);
    }

  std::size_t lrs_count = lrs.size();

  _ig = std::make_unique<Graph>(lrs_count);
  for (std::size_t i = 0; i < lrs_count; ++i)
    _ig->labels_set_vertex_name(i, "lr" + std::to_string(i));

  const auto &live_now = fun().get_analysis<LiveNow>();

  for (auto bb : fun().bbs()) {
    for (std::size_t i = 0; i < bb->code().size(); ++i) {
      isa::Ins cins(fun().parent().ctx(), bb->code()[i]);
      auto defs = cins.args_defs();
      if (defs.size() != 1)
        continue;

      assert(defs[0].substr(0, 2) == "lr");
      auto def_id = std::atoi(defs[0].c_str() + 2);

      auto lives = live_now.live_after(*bb, i);
      for (auto l : lives) {
        if (l == "sp")
          continue;

        // If there is a mov, there is no interference between use and def,
        // because both regs hold the same value
        if (cins.opname() == "mov" && l == cins.args()[2].substr(1))
          continue;

        assert(l.substr(0, 2) == "lr");
        auto l_id = std::atoi(l.c_str() + 2);
        if (def_id != l_id)
          _ig->add_edge(def_id, l_id);
      }
    }
  }

  _ig->dump_tree(*_doc);
}
