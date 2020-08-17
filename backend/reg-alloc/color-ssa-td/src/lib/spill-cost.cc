#include "spill-cost.hh"

#include <cstdlib>

#include "../isa/isa.hh"
#include "block-freq.hh"
#include "live-now.hh"
#include <logia/program.hh>

namespace {

constexpr double SPILL_LOAD_COST = 3;
constexpr double SPILL_STORE_COST = 4;

} // namespace

SpillCost::SpillCost(const isa::Function &fun) : isa::FunctionAnalysis(fun) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>("Spill Costs: @" +
                                                             fun.name());
  _build();
  _doc = nullptr;
}

void SpillCost::_build() {
  // Estimate the spill cost of every live range
  // This if the sum of every spill operation cost performed for all regs in a
  // live range
  // The sum of operation is the sum of store for def, load for use,
  // multiplied by the block execution frequency

  // Count number of live ranges
  std::set<std::string> lrs;
  for (auto bb : fun().bbs())
    for (const auto &ins : bb->code()) {
      isa::Ins cins(fun().parent().ctx(), ins);
      for (const auto &r : cins.args_uses())
        if (r.size() > 2 && r[0] == 'l' && r[1] == 'r')
          lrs.insert(r);
      for (const auto &r : cins.args_defs())
        if (r.size() > 2 && r[0] == 'l' && r[1] == 'r')
          lrs.insert(r);
    }

  std::size_t lrs_count = lrs.size();

  // Init all costs to 0
  _spill_costs.assign(lrs_count, 0.);

  const auto &bf = fun().get_analysis<BlockFreq>();

  // Sum cost of all operations in code
  for (auto bb : fun().bbs()) {
    auto freq = bf.freq(*bb);
    for (const auto &ins : bb->code()) {
      isa::Ins cins(fun().parent().ctx(), ins);

      for (const auto &r : cins.args_uses())
        if (r != "sp")
          _spill_costs[std::atoi(r.c_str() + 2)] += freq * SPILL_LOAD_COST;

      for (const auto &r : cins.args_defs())
        if (r != "sp")
          _spill_costs[std::atoi(r.c_str() + 2)] += freq * SPILL_STORE_COST;
    }
  }

  const auto &ln = fun().get_analysis<LiveNow>();

  // Figure out which LR have an infinite life cost
  for (std::size_t i = 0; i < _spill_costs.size(); ++i) {
    auto reg = "lr" + std::to_string(i);

    auto defs = ln.defs_pos_of(reg);
    auto ends = ln.ends_pos_of(reg);
    if (defs.size() != 1 || ends.size() != 1) // only handle simple cases
      continue;

    auto def = defs.front();
    auto end = ends.front();
    if (def.bb != end.bb) // if in another bb, assume probably may spill
      continue;

    assert(def.ins_pos < end.ins_pos);

    bool may_spill = false;

    for (std::size_t j = 0; j < _spill_costs.size(); ++j) {
      auto other_reg = "lr" + std::to_string(j);
      for (auto other_end : ln.ends_pos_of(other_reg))
        if (other_end.bb == def.bb && other_end.ins_pos > def.ins_pos &&
            other_end.ins_pos < end.ins_pos)
          may_spill = true;
    }

    if (!may_spill)
      _spill_costs[i] = SPILL_COST_INF;
  }

  *_doc << "## Estimated spill costs\n";
  for (std::size_t i = 0; i < _spill_costs.size(); ++i)
    *_doc << "- `LR" << i << ": "
          << (_spill_costs[i] == SPILL_COST_INF
                  ? "INF"
                  : std::to_string(_spill_costs[i]))
          << "`\n";
}
