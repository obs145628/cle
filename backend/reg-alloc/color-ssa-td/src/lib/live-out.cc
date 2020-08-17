#include "live-out.hh"

#include "../isa/isa.hh"
#include <logia/program.hh>

namespace {} // namespace

LiveOut::LiveOut(const isa::Function &fun)
    : isa::FunctionAnalysis(fun), _cfg(fun.get_analysis<CFG>()) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>("LiveOut: @" +
                                                             fun.name());
  _build();
  _doc = nullptr;
}

void LiveOut::_build() {
  _init();
  std::size_t niter = 0;
  _dump(niter++);
  for (;;) {
    if (!_update_all())
      break;
    _dump(niter++);
  }
}

void LiveOut::_init() {
  // Compute uevar and varkill for every bb

  for (auto bb : fun().bbs()) {
    auto &uev = _uevar[bb];
    auto &vk = _varkill[bb];

    for (const auto &ins : bb->code()) {
      isa::Ins cins(fun().parent().ctx(), ins);

      for (const auto &u : cins.args_uses())
        if (!vk.count(u)) {
          uev.insert(u);
          // Add uevar to liveouts of every pred
          for (auto pred : _cfg.preds(*bb))
            _liveout[pred].insert(u);
        }

      for (const auto &d : cins.args_defs())
        vk.insert(d);
    }
  }

  _dump_init();
}

bool LiveOut::_update(const isa::BasicBlock &bb) {
  bool changed = false;
  auto &lo = _liveout[&bb];

  for (auto succ : _cfg.succs(bb)) {
    const auto &vk = _varkill.at(succ);
    for (const auto &r : _liveout[succ])
      if (!vk.count(r) && !lo.count(r)) {
        lo.insert(r);
        changed = true;
      }
  }

  return changed;
}

bool LiveOut::_update_all() {
  bool changed = false;
  for (auto bb : fun().bbs())
    if (_update(*bb))
      changed = true;
  return changed;
}

void LiveOut::_dump_init() {
  _doc->raw_os() << "## UEVar (Upward Exposed Register Uses)\n";
  for (auto bb : fun().bbs()) {
    const auto &lo = _uevar[bb];
    _doc->raw_os() << " - " << bb->name() << ": `{";
    for (const auto &r : lo)
      _doc->raw_os() << r << "; ";
    _doc->raw_os() << "}`\n";
  }

  _doc->raw_os() << "## VarKill (Register Defs)\n";
  for (auto bb : fun().bbs()) {
    const auto &lo = _varkill[bb];
    _doc->raw_os() << " - " << bb->name() << ": `{";
    for (const auto &r : lo)
      _doc->raw_os() << r << "; ";
    _doc->raw_os() << "}`\n";
  }
}

const isa::BasicBlock *LiveOut::bb_from_name(const std::string &name) {
  for (auto bb : fun().bbs())
    if (bb->name() == name)
      return bb;
  assert(0);
}

void LiveOut::_dump(std::size_t niter) {
  _doc->raw_os() << "## Iteration " << niter << "\n";
  for (auto bb : fun().bbs()) {
    const auto &lo = _liveout[bb];
    _doc->raw_os() << " - " << bb->name() << ": `{";
    for (const auto &r : lo)
      _doc->raw_os() << r << "; ";
    _doc->raw_os() << "}`\n";
  }
}
