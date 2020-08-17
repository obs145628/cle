#include "live-now.hh"

#include "../isa/isa.hh"
#include <logia/program.hh>

namespace {
std::ostream &operator<<(std::ostream &os, const LiveNow::set_t &set) {
  os << "{";
  std::size_t rem = set.size();
  for (const auto &it : set) {
    os << it;
    if (--rem)
      os << ", ";
  }

  return os << "}";
}

std::ostream &operator<<(std::ostream &os,
                         const std::vector<LiveNow::Pos> &pos) {
  for (const auto &p : pos)
    os << "(" << p.bb->name() << ", " << p.ins_pos << ") ";
  return os;
}

} // namespace

LiveNow::LiveNow(const isa::Function &fun)
    : isa::FunctionAnalysis(fun), _liveout(fun.get_analysis<LiveOut>()) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>("LiveNow: @" +
                                                             fun.name());
  _build();
  _dump();
  _doc = nullptr;
}

void LiveNow::_build() {
  for (auto bb : fun().bbs())
    _build(*bb);
}

void LiveNow::_build(const isa::BasicBlock &bb) {
  auto &lives = _livenow[&bb];
  auto now = _liveout.liveout(bb);
  lives.resize(bb.code().size() + 1);
  lives.back() = now;

  // Start from liveout: livenow after last instruction
  // Compute livenow before each instruction from bottom to top
  // First remove defs (they are killed before the ins)
  // And add uses (they are live before the ins)
  // This order (defs then uses) is required for things such as add %x, %x, 2
  // It ensures x is still lives before, even if redefined

  for (std::size_t i = bb.code().size() - 1; i < bb.code().size(); --i) {
    isa::Ins cins(fun().parent().ctx(), bb.code()[i]);

    for (const auto &r : cins.args_defs()) {
      now.erase(r);
      _defs_pos[r].emplace_back(&bb, i);
    }
    for (const auto &r : cins.args_uses()) {
      if (!now.count(r))
        _ends_pos[r].emplace_back(&bb, i);
      now.insert(r);
    }

    lives[i] = now;
  }
}

void LiveNow::_dump() {

  {
    auto ch = _doc->code("asm");
    auto &os = ch.os();

    os << fun().name() << ":\n";
    os << ".fun\n\n";

    for (auto bb : fun().bbs()) {
      _dump(os, *bb);
      os << "\n";
    }
  }

  *_doc << "## Live Ranges\n";

  for (auto r : _defs_pos)
    *_doc << "- `Def(" << r.first << ") = " << r.second << "`\n";

  for (auto r : _ends_pos)
    *_doc << "- `End(" << r.first << ") = " << r.second << "`\n";
}

void LiveNow::_dump(std::ostream &os, const isa::BasicBlock &bb) {
  os << bb.name() << ":\n"
     << " ; Entry: " << live_before(bb, 0) << "\n";

  for (std::size_t i = 0; i < bb.code().size(); ++i) {
    bb.dump_ins(os, i);
    os << " ; " << live_after(bb, i) << "\n";
  }
}
