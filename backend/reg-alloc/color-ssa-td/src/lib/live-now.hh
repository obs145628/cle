#pragma once

#include <map>
#include <set>

#include "../isa/analysis.hh"
#include "../isa/module.hh"
#include "live-out.hh"
#include <logia/md-gfm-doc.hh>

class LiveNow : public isa::FunctionAnalysis {

public:
  using set_t = std::set<std::string>;
  using livenow_t = std::vector<set_t>;

  struct Pos {
    const isa::BasicBlock *bb;
    std::size_t ins_pos;

    Pos(const isa::BasicBlock *bb, std::size_t ins_pos)
        : bb(bb), ins_pos(ins_pos) {}
  };

  LiveNow(const isa::Function &fun);

  set_t live_before(const isa::BasicBlock &bb, std::size_t ins_pos) const {
    return live_after(bb, ins_pos - 1);
  }

  set_t live_after(const isa::BasicBlock &bb, std::size_t ins_pos) const {
    return _livenow.at(&bb).at(ins_pos + 1);
  }

  // Return the positions in code where reg is defined (live-range beg)
  std::vector<Pos> defs_pos_of(const std::string &reg) const {
    return _defs_pos.at(reg);
  }

  // Return the positions in code where reg is last used (live-range end)
  std::vector<Pos> ends_pos_of(const std::string &reg) const {
    auto it = _ends_pos.find(reg);
    return it == _ends_pos.end() ? std::vector<Pos>{} : it->second;
  }

private:
  const LiveOut &_liveout;
  std::map<const isa::BasicBlock *, livenow_t> _livenow;
  std::map<std::string, std::vector<Pos>> _defs_pos;
  std::map<std::string, std::vector<Pos>> _ends_pos;

  void _build();

  void _build(const isa::BasicBlock &bb);

  void _dump();
  void _dump(std::ostream &os, const isa::BasicBlock &bb);

  std::unique_ptr<logia::MdGfmDoc> _doc;
};
