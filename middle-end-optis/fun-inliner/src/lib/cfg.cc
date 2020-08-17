#include "cfg.hh"

#include <cstdlib>
#include <fstream>

#include "isa.hh"
#include "module.hh"
#include <utils/str/str.hh>

namespace {

std::vector<BasicBlock *> list_bbs(Function &fun) {
  std::vector<BasicBlock *> bbs;
  for (auto &bb : fun.bb())
    bbs.push_back(&bb);
  return bbs;
}

std::map<BasicBlock *, std::size_t>
bbs_map(const std::vector<BasicBlock *> &bbs) {
  std::map<BasicBlock *, std::size_t> res;
  for (std::size_t i = 0; i < bbs.size(); ++i)
    res.emplace(bbs[i], i);
  return res;
}

} // namespace

CFG::CFG(Function &fun)
    : _fun(fun), _bbs(list_bbs(_fun)), _inv(bbs_map(_bbs)),
      _graph(_bbs.size()) {
  _build_graph();
}

std::vector<BasicBlock *> CFG::preds(BasicBlock &bb) const {
  std::vector<BasicBlock *> res;
  for (auto v : _graph.preds(_inv.find(&bb)->second))
    res.push_back(_bbs.at(v));
  return res;
}

std::vector<BasicBlock *> CFG::succs(BasicBlock &bb) const {
  std::vector<BasicBlock *> res;
  for (auto v : _graph.succs(_inv.find(&bb)->second))
    res.push_back(_bbs.at(v));
  return res;
}

void CFG::_build_graph() {

  for (const auto &it : _inv) {
    auto &bb = it.first;
    auto idx = it.second;

    _graph.labels_set_vertex_name(idx, bb->label());

    const auto &bins = bb->ins().back();

    if (isa::is_branch(bins)) {
      for (auto t : isa::branch_targets(bins)) {
        _graph.add_edge(idx, _inv.find(_fun.get_bb(t))->second);
      }
    }

    std::ofstream ofs("cfg_" + _fun.name() + ".dot");
    _graph.dump_tree(ofs);
  }
}
