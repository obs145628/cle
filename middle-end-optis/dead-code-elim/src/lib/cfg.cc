#include "cfg.hh"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include "digraph-order.hh"

namespace {

std::vector<llvm::BasicBlock *> bbs_list(llvm::Function &fun) {
  std::vector<llvm::BasicBlock *> res;
  for (auto &bb : fun)
    res.push_back(&bb);
  return res;
}

Digraph build_graph(llvm::Function &fun,
                    const VertexAdapter<llvm::BasicBlock *> &va, bool reverse) {

  Digraph graph(va.size());

  for (auto &bb : fun) {

    graph.labels_set_vertex_name(va(&bb), bb.getName());
    auto bins = bb.getTerminator();

    if (bins->getOpcode() == llvm::Instruction::Br) {
      auto &br_ins = llvm::cast<llvm::BranchInst>(*bins);
      for (auto succ : br_ins.successors())
        graph.add_edge(va(&bb), va(succ));
    }

    else if (bins->getOpcode() == llvm::Instruction::Ret) {
      // terminal bb
    }

    else {
      llvm::errs() << "CFG: Unknown terminator ins: " << *bins << "\n";
      std::exit(1);
    }
  }

  Digraph res = reverse ? graph.reverse() : std::move(graph);

  std::ofstream ofs("cfg_" + std::string(fun.getName()) + ".dot");
  res.dump_tree(ofs);
  return res;
}

} // namespace

CFG::CFG(llvm::Function &fun, bool reverse)
    : _va(bbs_list(fun)), _graph(build_graph(fun, _va, reverse)) {}

std::vector<llvm::BasicBlock *> CFG::preds(llvm::BasicBlock &bb) const {
  std::vector<llvm::BasicBlock *> res;
  for (auto v : _graph.preds(_va(&bb)))
    res.push_back(_va(v));
  return res;
}

std::vector<llvm::BasicBlock *> CFG::succs(llvm::BasicBlock &bb) const {
  std::vector<llvm::BasicBlock *> res;
  for (auto v : _graph.succs(_va(&bb)))
    res.push_back(_va(v));
  return res;
}

std::vector<llvm::BasicBlock *> CFG::postorder() const {
  std::vector<llvm::BasicBlock *> res;
  for (auto v : ::postorder(_graph))
    res.push_back(_va(v));
  return res;
}

std::vector<llvm::BasicBlock *> CFG::rev_postorder() const {
  std::vector<llvm::BasicBlock *> res;
  for (auto v : ::rev_postorder(_graph))
    res.push_back(_va(v));
  return res;
}
