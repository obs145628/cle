#include "isa.hh"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <set>
#include <string>

#include <gop10/module.hh>
#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>
#include <utils/str/str.hh>

namespace isa {

namespace {

bool is_reg(const std::string &arg) { return arg.size() > 0 && arg[0] == '%'; }

bool is_const(const std::string &arg) {
  if (arg.empty())
    return false;
  for (std::size_t i = 0; i < arg.size(); ++i) {
    auto c = arg[i];
    if (c >= '0' && c <= '9')
      continue;
    if (c == '-' && i == 0)
      continue;
    return false;
  }

  return true;
}

bool is_block(const std::string &arg) {
  return arg.size() > 0 && arg[0] == '@';
}

bool is_fun(const std::string &arg) { return arg.size() > 0 && arg[0] == '@'; }

bool check_arg(const std::string &str, arg_kind_t arg) {
  if ((arg & ARG_KIND_REG) && is_reg(str))
    return true;
  if ((arg & ARG_KIND_CONST) && is_const(str))
    return true;
  if ((arg & ARG_KIND_BLOCK) && is_block(str))
    return true;
  if ((arg & ARG_KIND_FUN) && is_fun(str))
    return true;
  if ((arg & ARG_KIND_USE) && is_reg(str))
    return true;
  if ((arg & ARG_KIND_DEF) && is_reg(str))
    return true;
  if ((arg & ARG_KIND_USEDEF) && is_reg(str))
    return true;
  return false;
}

} // namespace

InsInfos::InsInfos(Context &ctx, const std::string &fmt) : _ctx(ctx) {

  auto fmt_args = utils::str::split(fmt, ' ');
  assert(fmt_args.front() == "@ins");
  fmt_args.erase(fmt_args.begin());

  assert(fmt_args.size() >= 2);

  if (fmt_args[0] == "call")
    _kind = InsKind::CALL;
  else if (fmt_args[0] == "ret")
    _kind = InsKind::RET;
  else if (fmt_args[0] == "branch")
    _kind = InsKind::BRANCH;
  else if (fmt_args[0] == "normal")
    _kind = InsKind::NORMAL;
  else
    assert(0);

  _opname = fmt_args[1];

  for (std::size_t i = 2; i < fmt_args.size(); ++i) {
    auto fmt_arg = utils::str::split(fmt_args[i], '|');
    auto kind = ARG_KIND_NONE;
    for (const auto &f : fmt_arg) {
      if (f == "r")
        kind |= ARG_KIND_REG;
      else if (f == "c")
        kind |= ARG_KIND_CONST;
      else if (f == "b")
        kind |= ARG_KIND_BLOCK;
      else if (f == "f")
        kind |= ARG_KIND_FUN;
      else if (f == "u")
        kind |= ARG_KIND_USE;
      else if (f == "d")
        kind |= ARG_KIND_DEF;
      else if (f == "x")
        kind |= ARG_KIND_USEDEF;
      else if (f == "*")
        kind |= ARG_KIND_REPEAT;
      else
        assert(0);
    }

    if (kind & ARG_KIND_REPEAT)
      assert(kind == ARG_KIND_REPEAT);

    _args.push_back(kind);
  }
}

bool InsInfos::is_variadic() const {
  return !_args.empty() && _args.back() == ARG_KIND_REPEAT;
}

arg_kind_t InsInfos::get_arg(std::size_t pos) const {
  assert(pos < _args.size() || is_variadic());
  if (is_variadic() && pos >= _args.size() - 1)
    return _args[_args.size() - 2];
  else
    return _args[pos];
}

bool InsInfos::has_def() const {
  for (std::size_t i = 0; i < _args.size(); ++i) {
    auto kind = get_arg(i);
    if (kind == ARG_KIND_DEF || kind == ARG_KIND_USEDEF)
      return true;
  }

  return false;
}

Context::Context(const std::string &isa_path) {
  std::ifstream ifs(isa_path);
  PANIC_IF(!ifs.good(),
           FMT_OSS("Failed to open isa file `" << isa_path << "'"));
  std::string l;
  while (std::getline(ifs, l)) {
    l = utils::str::trim(l);
    if (l.empty())
      continue;

    if (l.find("@ins") == 0) {
      _ins_infos.push_back(std::make_unique<InsInfos>(*this, l));
      auto infos = _ins_infos.back().get();
      assert(_infos_map.emplace(infos->opname(), infos).second);
    } else {
      PANIC(FMT_OSS("ISA context file: invalid line `" << l << "'"));
    }
  }
}

bool Context::is_instruction(const std::string &opname) const {
  return _infos_map.find(opname) != _infos_map.end();
}

const InsInfos &Context::get_infos(const std::string &opname) const {
  auto it = _infos_map.find(opname);
  PANIC_IF(it == _infos_map.end(),
           FMT_OSS("Unknown instruction `" << opname << "'"));
  return *it->second;
}

void Context::add_reg(const std::string &name) {
  auto reg = std::make_unique<RegInfos>(name);
  auto ptr = reg.get();
  _regs.push_back(std::move(reg));
  assert(_regs_map.emplace(name, ptr).second);
}

std::vector<const RegInfos *> Context::regs() const {
  std::vector<const RegInfos *> res;
  for (const auto &r : _regs)
    res.push_back(r.get());
  return res;
}

const RegInfos &Context::get_reg(const std::string &name) const {
  return *_regs_map.at(name);
}

const InsInfos &Ins::infos() const { return _ctx.get_infos(opname()); }

InsKind Ins::kind() const { return infos().kind(); }

bool Ins::is_term() const {
  return kind() == InsKind::BRANCH || kind() == InsKind::RET;
}

arg_kind_t Ins::get_arg_kind(std::size_t idx) const {
  auto kind = infos().get_arg(idx);
  const auto &arg = _args[idx + 1];
  if (is_block(arg))
    return ARG_KIND_BLOCK;
  else if (is_const(arg))
    return ARG_KIND_CONST;
  else if (is_const(arg))
    return ARG_KIND_FUN;
  else if (is_reg(arg)) {
    if (kind & ARG_KIND_USEDEF)
      return ARG_KIND_USEDEF;
    if (kind & ARG_KIND_DEF)
      return ARG_KIND_DEF;
    else if (kind & ARG_KIND_USE)
      return ARG_KIND_USE;
    else
      return ARG_KIND_REG;
  }

  assert(0);
}

void Ins::check() const {
  const auto &infos = _ctx.get_infos(opname());

  if (!infos.is_variadic() && infos.args().size() != (_args.size() - 1))
    PANIC(FMT_OSS("Invalid instruction "
                  << *this << ": expected " << infos.args().size()
                  << " arguments, got " << (_args.size() - 1)));
  if (infos.is_variadic() && (_args.size() - 1) < (infos.args().size() - 1))
    PANIC(FMT_OSS("Invalid instruction "
                  << *this << ": expected at least" << (infos.args().size() - 1)
                  << " arguments, got " << (_args.size() - 1)));

  for (std::size_t i = 1; i < _args.size(); ++i)
    if (!check_arg(_args[i], infos.get_arg(i - 1)))
      PANIC(FMT_OSS("Invalid instruction " << *this << ": arg #" << i
                                           << " is invalid"));
}

std::vector<std::string> Ins::target_blocks() const {
  assert(kind() == InsKind::BRANCH);
  std::vector<std::string> res;
  for (std::size_t i = 1; i < _args.size(); ++i)
    if (infos().get_arg(i - 1) == ARG_KIND_BLOCK)
      res.push_back(_args[i].substr(1));
  return res;
}

std::vector<std::string> Ins::args_uses() const {
  std::vector<std::string> res;
  for (std::size_t i = 1; i < _args.size(); ++i) {
    auto kind = get_arg_kind(i - 1);
    auto r = _args[i].substr(1);
    if ((kind == ARG_KIND_USE || kind == ARG_KIND_USEDEF) &&
        std::find(res.begin(), res.end(), r) == res.end())
      res.push_back(r);
  }
  return res;
}

std::vector<std::string> Ins::args_defs() const {
  std::vector<std::string> res;
  for (std::size_t i = 1; i < _args.size(); ++i) {
    auto kind = get_arg_kind(i - 1);
    if (kind == ARG_KIND_DEF || kind == ARG_KIND_USEDEF)
      res.push_back(_args[i].substr(1));
  }
  return res;
}

std::ostream &operator<<(std::ostream &os, const Ins &ins) {
  for (std::size_t i = 0; i < ins.args().size(); ++i) {
    os << ins.args()[i];
    if (i + 1 < ins.args().size())
      os << (i == 0 ? " " : ", ");
  }
  return os;
}

void check(const Context &ctx, const gop::Module &mod) {

#define LC_ERR(Mess) PANIC(FMT_OSS(Mess))

  // prototypes of all defined functions
  std::vector<std::size_t> _funs_pos;

  // Build prototypes and ranges for all functions
  for (std::size_t i = 0; i < mod.decls.size(); ++i) {
    auto dir = dynamic_cast<gop::Dir *>(mod.decls[i].get());

    if (dir && dir->args[0] == "fun") {
      if (dir->label_defs.size() != 1)
        LC_ERR("fun directive requires one label");

      _funs_pos.push_back(i);
    }
  }
  _funs_pos.push_back(mod.decls.size());

  // Check all functions one by one
  for (std::size_t i = 0; i + 1 < _funs_pos.size(); ++i) {
    auto beg = &mod.decls[_funs_pos[i] + 1];
    auto end = &mod.decls[_funs_pos[i + 1]];
    auto fun_dir = dynamic_cast<gop::Dir *>((beg - 1)->get());
    assert(fun_dir);

    // First pass to find function infos
    std::set<std::string> labels;

#if 0
    // Need infos like regs valid at function entry to make this work
    std::set<std::string> defs;
    for (const auto &r : fundecl_args(fun_dir->args))
      defs.insert(r);
#endif

    for (auto it = beg; it != end; ++it) {
      auto ins = dynamic_cast<gop::Ins *>(it->get());
      assert(ins);
      Ins cins(ctx, ins->args);
      cins.check();

      for (auto &lbl : ins->label_defs)
        if (!labels.insert(lbl).second)
          LC_ERR("Multiple definition of label `" << lbl << "' at `" << cins
                                                  << "'");

      ;

#if 0
        if (def_idx(ins->args) != IDX_NO)
          defs.insert(get_def(ins->args));
#endif
    }

    // Second pass to check all instructions
    for (auto it = beg; it != end; ++it) {
      auto ins = dynamic_cast<gop::Ins *>(it->get());
      auto dir = dynamic_cast<gop::Dir *>(it->get());
      Ins cins(ctx, ins->args);
      assert(!dir);
      assert(ins);

      if (cins.kind() == InsKind::BRANCH) {
        for (const auto &t : cins.target_blocks())
          if (!labels.count(t))
            LC_ERR("Branching to undefined label `" << t << "' at `" << cins
                                                    << "'");
      }

#if 0
      for (const auto &r : get_uses(ins->args))
        if (!defs.count(r))
          LC_ERR("Use of undefined register `" << r << "' at `" << cins << "'");
#endif
    }
  }

#undef LC_ERR
}

} // namespace isa
