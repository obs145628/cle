#include "cfg.hh"

#include <cstdlib>
#include <fstream>
#include <iostream>

#include "../isa/isa.hh"
#include "cfg.hh"
#include "digraph-order.hh"
#include <utils/str/str.hh>

CFG::CFG(const Function &fun)
    : _fun(fun), _va(fun.bb().map([](const BasicBlock &bb) { return &bb; })),
      _graph(_va.size()) {
  _build_graph();
}

std::vector<BasicBlock *> CFG::preds(BasicBlock &bb) {
  auto &mva = _get_mva(bb);
  std::vector<BasicBlock *> res;
  for (auto v : _graph.preds(mva(&bb)))
    res.push_back(mva(v));
  return res;
}

std::vector<const BasicBlock *> CFG::preds(const BasicBlock &bb) const {
  std::vector<const BasicBlock *> res;
  for (auto v : _graph.preds(_va(&bb)))
    res.push_back(_va(v));
  return res;
}

std::vector<BasicBlock *> CFG::succs(BasicBlock &bb) {
  auto &mva = _get_mva(bb);
  std::vector<BasicBlock *> res;
  for (auto v : _graph.succs(mva(&bb)))
    res.push_back(mva(v));
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
    _graph.labels_set_vertex_name(_va(&bb), bb.get_name());
    const auto &bins = bb.ins().back();

    for (auto succ : bins.branch_targets())
      _graph.add_edge(_va(&bb), _va(succ));
  }

  std::ofstream ofs("cfg_" + std::string(_fun.get_name()) + ".dot");
  _graph.dump_tree(ofs);
}

VertexAdapter<BasicBlock *> &CFG::_get_mva(BasicBlock &bb) {
  auto &fun = bb.parent();
  assert(&fun == &_fun);
  if (_mva.get())
    return *_mva;

  _mva = std::make_unique<VertexAdapter<BasicBlock *>>(
      fun.bb().map([](BasicBlock &bb) { return &bb; }));
  return *_mva;
}
