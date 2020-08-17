#pragma once

#include "analysis.hh"

#include "../utils/digraph.hh"
#include "../utils/vertex-adapter.hh"
#include "module.hh"

class CFG : public FunctionAnalysis {

public:
  CFG(const Function &fun);
  CFG(const CFG &) = delete;
  CFG &operator=(const CFG &) = delete;

  const Digraph &get_graph() const { return _g; }

  std::vector<const BasicBlock *> preds(const BasicBlock &bb) const;
  std::vector<const BasicBlock *> succs(const BasicBlock &bb) const;

private:
  const VertexAdapter<const BasicBlock *> _va;
  const Digraph _g;
};
