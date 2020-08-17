#include "cfg.hh"

#include <cstdlib>
#include <fstream>
#include <iostream>

#include "cfg.hh"
#include "digraph-order.hh"
#include "isa.hh"
#include <utils/str/str.hh>

CFG::CFG(const Function &fun)
    : _fun(fun), _va(fun.bb().map([](const BasicBlock &bb) { return &bb; })),
      _graph(_va.size()) {
  _build_graph();
}

std::vector<const BasicBlock *> CFG::preds(const BasicBlock &bb) const {
  std::vector<const BasicBlock *> res;
  for (auto v : _graph.preds(_va(&bb)))
    res.push_back(_va(v));
  return res;
}

std::vector<const BasicBlock *> CFG::succs(const BasicBlock &bb) const {
  std::vector<const BasicBlock *> res;
  for (auto v : _graph.succs(_va(&bb)))
    res.push_back(_va(v));
  return res;
}

std::vector<const BasicBlock *> CFG::rev_postorder() const {
  std::vector<const BasicBlock *> res;
  for (auto v : digraph_dfs(_graph, DFSOrder::REV_POST))
    res.push_back(_va(v));
  return res;
}

void CFG::_build_graph() {
  for (const auto &bb : _fun.bb()) {
    _graph.labels_set_vertex_name(_va(&bb), bb.label());
    const auto &bins = bb.ins().back();

    for (const auto &succ : isa::branch_targets(bins)) {
      _graph.add_edge(_va(&bb), _va(_fun.get_bb(succ)));
    }
  }

  std::ofstream ofs("cfg_" + std::string(_fun.name()) + ".dot");
  _graph.dump_tree(ofs);
}
