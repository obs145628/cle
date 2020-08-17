#pragma once

#include <memory>

#include "../isa/analysis.hh"
#include "../isa/module.hh"
#include "../utils/graph.hh"
#include <logia/md-gfm-doc.hh>

class InterferenceGraph : public isa::FunctionAnalysis {

public:
  InterferenceGraph(const isa::Function &fun);

  const Graph &graph() const { return *_ig; }

private:
  std::unique_ptr<Graph> _ig;

  std::unique_ptr<logia::MdGfmDoc> _doc;

  void _build();
};
