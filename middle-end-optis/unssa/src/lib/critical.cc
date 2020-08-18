#include "critical.hh"

#include <iostream>

#include "../isa/isa.hh"
#include "cfg.hh"

namespace {

class CritSplit {
public:
  CritSplit(Function &fun) : _fun(fun), _cfg(_fun) {}

  void run() {

    std::vector<std::pair<BasicBlock *, BasicBlock *>> crits;

    std::cerr << "Critical edges in " << _fun.get_name() << ":\n";
    for (auto &src : _fun.bb()) {
      for (auto dst : _cfg.succs(src))
        if (is_crit(src, *dst)) {
          crits.emplace_back(&src, dst);
          std::cerr << "  " << src.get_name() << " --> " << dst->get_name()
                    << "\n";
        }
    }

    for (auto p : crits)
      split(*p.first, *p.second);
  }

private:
  Function &_fun;
  CFG _cfg;

  bool is_crit(const BasicBlock &src, const BasicBlock &dst) {
    return _cfg.succs(src).size() > 1 && _cfg.preds(dst).size() > 1;
  }

  void split(BasicBlock &src, BasicBlock &dst) {

    // Create block split_bb with only 1 ins: b dst
    BasicBlock &split_bb = _fun.add_bb();
    split_bb.insert_ins(split_bb.ins_end(), "b", {&dst}, "", isa::IDX_NO);

    // Replace jump to dst by a jump to split_bb at src terminator
    auto &bins = src.ins().back();
    assert(bins.get_opname() == "bc");
    std::size_t pos = bins.ops_count();
    for (std::size_t i = 0; i < bins.ops_count(); ++i)
      if (&dst == &bins.op(i)) {
        pos = i;
        break;
      }
    assert(pos < bins.ops_count());

    bins.set_op(pos, split_bb);

    // Replace src by split_bb in all phis in dst
    for (auto &ins : dst.ins()) {
      if (ins.get_opname() != "phi")
        break;
      for (std::size_t i = 0; i < ins.ops_count(); i += 2)
        if (&ins.op(i) == &src)
          ins.set_op(i, split_bb);
    }
  }
};

} // namespace

void critical_split(Module &mod) {
  for (auto &fun : mod.fun()) {
    if (!fun.has_def())
      continue;
    CritSplit cs(fun);
    cs.run();
  }
}
