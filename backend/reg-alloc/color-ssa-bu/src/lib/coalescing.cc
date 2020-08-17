#include "coalescing.hh"

#include <cstdlib>

#include "interference-graph.hh"
#include "live-now.hh"
#include "live-out.hh"
#include <logia/program.hh>

Coalescing::Coalescing(isa::Function &fun)
    : _fun(fun), _ctx(_fun.parent().ctx()) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(
      "Coalescing for @" + _fun.name());
}

void Coalescing::run() {
  // Keep coaslesing until not possible
  while (true) {
    std::size_t lr1;
    std::size_t lr2;
    if (!_find_next(lr1, lr2))
      break;

    _coalesce(lr1, lr2);
  }

  *_doc << "## Coalesced code\n";
  _fun.dump_code(*_doc);
}

bool Coalescing::_find_next(std::size_t &lr1, std::size_t &lr2) {
  // Try to find 2 live ranges that can be coalesced.
  // Is successfull, return true, and store live-ranges in lr1 and lr2

  const auto &ig = _fun.get_analysis<InterferenceGraph>().graph();

  for (auto bb : _fun.bbs())
    for (const auto &ins : bb->code()) {
      if (ins[0] != "mov")
        continue;
      if (ins[1].substr(0, 3) != "%lr" || ins[2].substr(0, 3) != "%lr")
        continue;

      lr1 = std::atoi(ins[1].c_str() + 3);
      lr2 = std::atoi(ins[2].c_str() + 3);
      if (!ig.has_edge(lr1, lr2))
        return true;
    }

  return false;
}

void Coalescing::_coalesce(std::size_t lr_def, std::size_t lr_use) {
  *_doc << "- Coalescing `mov %lr" << lr_def << ", %lr" << lr_use << "`\n";

  std::size_t lr_count = _fun.get_analysis<InterferenceGraph>().graph().v();

  // First delete the instruction
  for (auto bb : _fun.bbs())
    for (auto it = bb->code().begin(); it != bb->code().end(); ++it) {
      const auto &ins = *it;
      if (ins[0] != "mov")
        continue;
      if (ins[1].substr(0, 3) != "%lr" || ins[2].substr(0, 3) != "%lr")
        continue;

      std::size_t lr1 = std::atoi(ins[1].c_str() + 3);
      std::size_t lr2 = std::atoi(ins[2].c_str() + 3);

      if (lr1 == lr_def && lr2 == lr_use) {
        bb->code().erase(it);
        break;
      }
    }

  // Replace all refs to the def by refs to the use
  _rename_lr(lr_def, lr_use);

  // lr_def is unused now
  // but it may leave a "hole" in the lrs list
  // replace last one with lr_def to fix this
  if (lr_def + 1 != lr_count)
    _rename_lr(lr_count - 1, lr_def);

  // Many informations became invalid now
  _fun.invalidate_analysis<InterferenceGraph>();
  _fun.invalidate_analysis<LiveOut>();
  _fun.invalidate_analysis<LiveNow>();
}

void Coalescing::_rename_lr(std::size_t lr_old, std::size_t lr_new) {
  // Replace all refs of lr_old by lr_new
  auto reg_old = "%lr" + std::to_string(lr_old);
  auto reg_new = "%lr" + std::to_string(lr_new);

  for (auto bb : _fun.bbs())
    for (auto &ins : bb->code())
      for (auto &arg : ins)
        if (arg == reg_old)
          arg = reg_new;
}
