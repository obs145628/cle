#include "cfg.hh"

#include <cstdlib>
#include <fstream>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include "digraph-order.hh"
#include <utils/str/str.hh>

namespace {

std::vector<const llvm::BasicBlock *> list_bbs(const llvm::Function &fun) {
  std::vector<const llvm::BasicBlock *> bbs;
  for (auto &bb : fun)
    bbs.push_back(&bb);
  return bbs;
}

std::map<const llvm::BasicBlock *, std::size_t>
bbs_map(std::vector<const llvm::BasicBlock *> &bbs) {
  std::map<const llvm::BasicBlock *, std::size_t> res;
  for (std::size_t i = 0; i < bbs.size(); ++i)
    res.emplace(bbs[i], i);
  return res;
}

} // namespace

CFG::CFG(const llvm::Function &fun)
    : _fun(fun), _bbs(list_bbs(_fun)), _inv(bbs_map(_bbs)),
      _graph(_bbs.size()) {
  _build_graph();
}

std::vector<const llvm::BasicBlock *>
CFG::preds(const llvm::BasicBlock &bb) const {
  std::vector<const llvm::BasicBlock *> res;
  for (auto v : _graph.preds(_inv.find(&bb)->second))
    res.push_back(_bbs.at(v));
  return res;
}

std::vector<const llvm::BasicBlock *>
CFG::succs(const llvm::BasicBlock &bb) const {
  std::vector<const llvm::BasicBlock *> res;
  for (auto v : _graph.succs(_inv.find(&bb)->second))
    res.push_back(_bbs.at(v));
  return res;
}

std::vector<const llvm::BasicBlock *> CFG::rev_postorder() const {
  std::vector<const llvm::BasicBlock *> res;
  for (auto v : ::rev_postorder(_graph))
    res.push_back(_bbs.at(v));
  return res;
}

void CFG::_build_graph() {

  for (const auto &it : _inv) {
    auto &bb = it.first;
    auto idx = it.second;

    _graph.labels_set_vertex_name(idx, bb->getName());

    auto bins = bb->getTerminator();

    if (bins->getOpcode() == llvm::Instruction::Br) {
      const auto &br_ins = llvm::cast<llvm::BranchInst>(*bins);
      for (auto succ : br_ins.successors())
        _graph.add_edge(idx, _inv.find(succ)->second);
    }

    else if (bins->getOpcode() == llvm::Instruction::Ret) {
      // terminal bb
    }

    else {
      llvm::errs() << "CFG: Unknown terminator ins: " << *bins << "\n";
      std::exit(1);
    }
  }

  std::ofstream ofs("cfg_" + std::string(_fun.getName()) + ".dot");
  _graph.dump_tree(ofs);
}
