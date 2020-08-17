#pragma once

#include <map>
#include <set>

#include "analysis.hh"
#include "cfg.hh"
#include "module.hh"
#include <logia/md-gfm-doc.hh>

class LiveOut : public FunctionAnalysis {

public:
  using set_t = std::set<std::string>;

  LiveOut(const Function &fun);

  const set_t &liveout(const BasicBlock &bb) const { return _liveout.at(&bb); }
  const set_t &uevar(const BasicBlock &bb) const { return _uevar.at(&bb); }
  const set_t &varkill(const BasicBlock &bb) const { return _varkill.at(&bb); }

private:
  const CFG &_cfg;
  std::map<const BasicBlock *, set_t> _liveout;
  std::map<const BasicBlock *, set_t> _uevar;
  std::map<const BasicBlock *, set_t> _varkill;

  std::unique_ptr<logia::MdGfmDoc> _doc;

  void _build();
  void _init();

  bool _update(const BasicBlock &bb);
  bool _update_all();

  void _dump_init();
  void _dump(std::size_t niter);
};
