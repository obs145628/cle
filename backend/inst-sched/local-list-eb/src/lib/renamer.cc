#include "renamer.hh"

#include <cassert>

#include "renamer.hh"
#include <logia/md-gfm-doc.hh>
#include <logia/program.hh>

namespace {

void log_mod(const std::string &title, const isa::Module &m) {
  auto doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(title);
  auto ch = doc->code("asm");
  m.dump_code(std::cout);
  m.dump_code(ch.os());
}

} // namespace

Renamer::Renamer(isa::Module &mod) : _mod(mod), _ctx(mod.ctx()) {}

void Renamer::run() {
  log_mod("Input IR", _mod);

  for (auto f : _mod.funs()) {
    _next_reg = 0;
    _liveouts[f] = &f->get_analysis<LiveOut>();
    for (auto bb : f->bbs())
      _udp_bb(*bb);

    f->invalidate_analysis<LiveOut>();
  }

  log_mod("Renamed IR", _mod);
  _mod.check();
}

void Renamer::_udp_bb(isa::BasicBlock &bb) {
  const auto &out_bb = _liveouts.at(&bb.parent())->liveout(bb);

  for (std::size_t i = 0; i < bb.code().size(); ++i) {
    isa::Ins ins(_ctx, bb.code()[i]);
    auto defs = ins.args_defs();
    if (defs.size() != 1)
      continue;

    auto old_name = defs[0];

    if (out_bb.count(old_name)) // live at end of bb, doesn't rename
      continue;

    auto new_name = _gen_reg_name();
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

std::string Renamer::_gen_reg_name() {
  return "t" + std::to_string(_next_reg++);
}
