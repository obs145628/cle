#pragma once

#include <cassert>
#include <map>
#include <vector>

#include "module.hh"

struct BB {
  std::size_t idx;
  std::size_t ins_beg;
  std::size_t ins_end;
};

struct BBList {
public:
  // build a sequence of basic blocks for mod
  // must respect bb rules:
  // - a bb must have at least one instruction
  // - a not-last instruction in a bb must not branch
  // - the last instruction in a bb must branch
  // - a branching must be to the beginning of a basic block
  // - an instruction must belong to a basic block
  BBList(const Module &mod);

  std::size_t count() const { return _bbs.size(); }

  const std::vector<BB> &all() const { return _bbs; }

  const BB &get(std::size_t idx) const { return _bbs.at(idx); }

  // Find basic block starting at instruction index ins_idx
  const BB get_at(std::size_t ins_idx) const {
    auto it = _begs_map.find(ins_idx);
    assert(it != _begs_map.end());
    return get(it->second);
  }

private:
  std::vector<BB> _bbs;
  std::map<std::size_t, std::size_t> _begs_map;

  void _check_label(const Module &mod, const std::string &lbl);
};
