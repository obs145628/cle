#pragma once

#include <map>
#include <set>

#include "../isa/analysis.hh"
#include "../isa/module.hh"
#include "cfg.hh"
#include <logia/md-gfm-doc.hh>

class LiveOut : public isa::FunctionAnalysis {

public:
  using set_t = std::set<std::string>;

  LiveOut(const isa::Function &fun);

  const set_t &liveout(const isa::BasicBlock &bb) const {
    return _liveout.at(&bb);
  }
  const set_t &uevar(const isa::BasicBlock &bb) const { return _uevar.at(&bb); }
  const set_t &varkill(const isa::BasicBlock &bb) const {
    return _varkill.at(&bb);
  }

private:
  const CFG &_cfg;
  std::map<const isa::BasicBlock *, set_t> _liveout;
  std::map<const isa::BasicBlock *, set_t> _uevar;
  std::map<const isa::BasicBlock *, set_t> _varkill;

  std::unique_ptr<logia::MdGfmDoc> _doc;

  void _build();
  void _init();

  bool _update(const isa::BasicBlock &bb);
  bool _update_all();

  void _dump_init();
  void _dump(std::size_t niter);
};
