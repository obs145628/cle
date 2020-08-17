#pragma once

#include <vector>

#include "../isa/analysis.hh"
#include "../isa/module.hh"
#include <logia/md-gfm-doc.hh>

constexpr double SPILL_COST_INF = 1e10;

class SpillCost : public isa::FunctionAnalysis {

public:
  SpillCost(const isa::Function &fun);

  std::size_t lr_count() const { return _spill_costs.size(); }
  double lr_cost(std::size_t idx) const { return _spill_costs.at(idx); }

private:
  std::vector<double> _spill_costs;

  std::unique_ptr<logia::MdGfmDoc> _doc;

  void _build();

  std::size_t _try_assign(std::size_t lr);
};
