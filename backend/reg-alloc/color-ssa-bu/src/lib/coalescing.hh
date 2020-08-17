#pragma once

#include <memory>

#include "../isa/module.hh"
#include <logia/md-gfm-doc.hh>

// Perform live-range coalescing
// 2 live ranges can be coalesced if there is an instruction:
// mov %lrd, %lru, where (%lrd, %lru) doesn't interfere
// the mov instruction is removed, and all refs to %lrd are replaced by %lru
//
// Coalescing 2 live range may allow further coalescing
// Algo stop when no coalescing is feasble anymore

class Coalescing {
public:
  Coalescing(isa::Function &fun);

  void run();

private:
  isa::Function &_fun;
  const isa::Context &_ctx;
  std::unique_ptr<logia::MdGfmDoc> _doc;

  bool _find_next(std::size_t &lr1, std::size_t &lr2);

  void _coalesce(std::size_t lr_def, std::size_t lr_use);

  void _rename_lr(std::size_t lr_old, std::size_t lr_new);
};
