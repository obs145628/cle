#include "renamer.hh"

#include <cassert>

Renamer::Renamer(isa::Module &mod) : _mod(mod), _ctx(mod.ctx()) {}

void Renamer::run() {
  for (auto f : _mod.funs())
    for (auto bb : f->bbs())
      _udp_bb(*bb);
}

void Renamer::_udp_bb(isa::BasicBlock &bb) {
  std::size_t next = 1;

  for (std::size_t i = 0; i < bb.code().size(); ++i) {
    isa::Ins ins(_ctx, bb.code()[i]);
    auto defs = ins.args_defs();
    if (defs.size() != 1)
      continue;

    auto old_name = defs[0];
    auto new_name = "t" + std::to_string(next++);
    _rename_def(bb.code()[i], new_name);
    _rename_uses(bb, old_name, new_name, i + 1);
  }
}

void Renamer::_rename_uses(isa::BasicBlock &bb, const std::string &old_name,
                           const std::string &new_name, std::size_t beg) {
  for (std::size_t i = beg; i < bb.code().size(); ++i) {
    isa::Ins ins(_ctx, bb.code()[i]);
    auto defs = ins.args_defs();

    _rename_uses(bb.code()[i], old_name, new_name);

    if (defs.size() == 1 && defs[0] == old_name)
      break;
  }
}

void Renamer::_rename_def(std::vector<std::string> &ins,
                          const std::string &new_name) {
  isa::Ins cins(_ctx, ins);
  for (std::size_t i = 1; i < ins.size(); ++i)
    if (cins.get_arg_kind(i - 1) == isa::ARG_KIND_DEF) {
      ins[i] = "%" + new_name;
      return;
    }

  assert(0);
}

void Renamer::_rename_uses(std::vector<std::string> &ins,
                           const std::string &old_name,
                           const std::string &new_name) {

  isa::Ins cins(_ctx, ins);
  for (std::size_t i = 1; i < ins.size(); ++i)
    if (cins.get_arg_kind(i - 1) == isa::ARG_KIND_USE &&
        ins[i] == ("%" + old_name))
      ins[i] = "%" + new_name;
}
