#pragma once

#include "../isa/analysis.hh"

#include "../isa/module.hh"
#include "../utils/digraph.hh"
#include "../utils/vertex-adapter.hh"

class CFG : public isa::FunctionAnalysis {

public:
  CFG(const isa::Function &fun);
  CFG(const CFG &) = delete;
  CFG &operator=(const CFG &) = delete;

  const Digraph &get_graph() const { return _g; }

  std::vector<const isa::BasicBlock *> preds(const isa::BasicBlock &bb) const;
  std::vector<const isa::BasicBlock *> succs(const isa::BasicBlock &bb) const;

private:
  const VertexAdapter<const isa::BasicBlock *> _va;
  const Digraph _g;
};
