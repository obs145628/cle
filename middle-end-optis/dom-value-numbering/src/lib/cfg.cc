#include "cfg.hh"

#include <cstdlib>
#include <fstream>
#include <iostream>

#include "../isa/isa.hh"
#include "cfg.hh"
#include "digraph-order.hh"
#include <utils/str/str.hh>

namespace {
Digraph build_graph(const Function &fun,
                    const VertexAdapter<const BasicBlock *> &va) {

  Digraph graph(va.size());

  for (const auto &bb : fun.bb()) {
    graph.labels_set_vertex_name(va(&bb), bb.get_name());
    const auto &bins = bb.ins().back();

    for (auto succ : bins.branch_targets())
      graph.add_edge(va(&bb), va(succ));
  }

  std::ofstream ofs("cfg_" + std::string(fun.get_name()) + ".dot");
  graph.dump_tree(ofs);
  return graph;
}
} // namespace

CFG::CFG(const Function &fun)
    : _fun(fun), _va(fun.bb().map([](const BasicBlock &bb) { return &bb; })),
      _graph(build_graph(_fun, _va)) {}

std::vector<BasicBlock *> CFG::preds(BasicBlock &bb) const {
  auto &mva = this->mva(bb);
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

std::vector<BasicBlock *> CFG::succs(BasicBlock &bb) const {
  auto &mva = this->mva(bb);
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

const VertexAdapter<BasicBlock *> &CFG::mva(Function &fun) const {
  assert(&fun == &_fun);
  if (_mva.get())
    return *_mva;

  _mva = std::make_unique<VertexAdapter<BasicBlock *>>(
      fun.bb().map([](BasicBlock &bb) { return &bb; }));
  return *_mva;
}
