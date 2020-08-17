#pragma once

#include "../isa/analysis.hh"

#include "../isa/module.hh"
#include "../utils/digraph.hh"
#include "../utils/vertex-adapter.hh"
#include "cfg.hh"
#include <logia/md-gfm-doc.hh>

#include <utility>

// Compute the execution frequency of each block
// The value is the expected number of times a block will be executed through
// one execution of the function
//
// It assumes for a conditional branch every target is 50/50
//
// @TODO: detect loop, suppose they are

class BlockFreq : public isa::FunctionAnalysis {

public:
  BlockFreq(const isa::Function &fun);
  BlockFreq(const BlockFreq &) = delete;
  BlockFreq &operator=(const BlockFreq &) = delete;

  double freq(const isa::BasicBlock &bb) const { return _freqs.at(&bb); }

private:
  using back_edge_t =
      std::pair<const isa::BasicBlock *, const isa::BasicBlock *>;

  const CFG &_cfg;
  std::vector<back_edge_t> _back_edges;

  std::unique_ptr<logia::MdGfmDoc> _doc;

  void _run();

  void _find_back_edges();
  void _find_back_edges_rec(const isa::BasicBlock &bb,
                            std::set<const isa::BasicBlock *> &visited,
                            std::set<const isa::BasicBlock *> &on_stack);

  std::set<const isa::BasicBlock *> _back_edge_preds(const isa::BasicBlock &bb);
  std::set<const isa::BasicBlock *> _back_edge_succs(const isa::BasicBlock &bb);

  void _eval(const isa::BasicBlock &bb, double freq);

  std::map<const isa::BasicBlock *, double> _freqs;
};
