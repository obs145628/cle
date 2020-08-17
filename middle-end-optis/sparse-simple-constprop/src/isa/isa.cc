#include "isa.hh"

#include <algorithm>
#include <cassert>
#include <map>
#include <set>

#include <gop10/module.hh>
#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>
#include <utils/str/str.hh>

// How to add a new instruction:
// - add syntax in ilist
// - for special instruction, add special cases code in InsInfos methods
// - for special instructions, add special cases code in check(Module)
// - add a HOOK_INS(xxx) and define r_xxx method in simplevm10.cc: Context class

namespace isa {

namespace {

bool is_reg(const std::string &arg) { return arg.size() > 1 && arg[0] == '%'; }

bool is_label(const std::string &arg) {
  return arg.size() > 1 && arg[0] == '@';
}

bool is_const(const std::string &arg) {
  if (arg.empty())
    return false;

  bool empty = true;

  for (std::size_t i = 0; i < arg.size(); ++i) {
    if (!i && arg[i] == '-')
      continue;
    if (arg[i] < '0' || arg[i] > '9')
      return false;
    empty = false;
  }

  return !empty;
}

bool is_val(const std::string &arg) { return is_reg(arg) || is_const(arg); }

// Definitions
// Name prefix:
// @: branch ins
//
// Arguments:
// d: def register
// v: const or use register
// t: target label
// *: one or many arguments
//
struct InsInfos {

  enum class Kind {
    BRANCH,
    NORMAL,
  };

  enum class ArgKind {
    ANY,
    DEF,
    VAL,
    TARGET,
  };

  static std::map<std::string, InsInfos *> _ins_map;

  InsInfos(const char *fmt) {

    auto specs = utils::str::split(fmt, ' ');
    kind = Kind::NORMAL;
    name = specs[0];
    nargs_min = specs.size();
    nargs_max = specs.size();
    args = {ArgKind::ANY};

    if (name[0] == '@') {
      kind = Kind::BRANCH;
      name = name.substr(1);
    }

    for (std::size_t i = 1; i < specs.size(); ++i) {
      if (specs[i] == "d")
        args.push_back(ArgKind::DEF);
      else if (specs[i] == "v")
        args.push_back(ArgKind::VAL);
      else if (specs[i] == "t")
        args.push_back(ArgKind::TARGET);
      else if (specs[i] == "*") {
        nargs_min = i;
        nargs_max = std::size_t(-1);
      } else
        assert(0);
    }

    _ins_map.emplace(name, this);
  };

  /// Check is the instruction syntax is correct
  /// Return empty if no error
  static std::string check(const def_t &ins) {
    for (const auto &arg : ins)
      if (arg.empty())
        return "Invalid instruction form: empty argument";

    auto it = _ins_map.find(ins[0]);
    if (it == _ins_map.end())
      return "Unknown instruction";

    return it->second->_check(ins);
  }

  static std::size_t def_idx(const def_t &ins) {
    auto it = _ins_map.find(ins[0]);
    assert(it != _ins_map.end());
    auto infos = it->second;

    std::size_t res = IDX_NO;
    for (std::size_t i = 0; i < infos->args.size(); ++i)
      if (infos->args[i] == ArgKind::DEF)
        res = i;

    // Special case for call
    if (is_call(ins) && call_has_ret(ins))
      res = 1;

    assert(res == IDX_NO || is_reg(ins[res]));
    return res;
  }

  static std::vector<std::size_t> uses_idxs(const def_t &ins) {
    auto it = _ins_map.find(ins[0]);
    assert(it != _ins_map.end());
    auto infos = it->second;

    // Special cases
    if (infos->name == "call")
      return _list_regs(ins, call_has_ret(ins) ? 2 : 0);
    if (infos->name == "phi")
      return _list_regs(ins, 2);
    if (infos->name == "ret")
      return _list_regs(ins);

    std::vector<std::size_t> res;
    for (std::size_t i = 0; i < infos->args.size(); ++i)
      if (infos->args[i] == ArgKind::VAL && is_reg(ins[i]))
        res.push_back(i);
    return res;
  }

  static bool is_branch(const def_t &ins) {
    auto it = _ins_map.find(ins[0]);
    assert(it != _ins_map.end());
    auto infos = it->second;
    return infos->kind == Kind::BRANCH;
  }

  static std::vector<std::size_t> targets_idxs(const def_t &ins) {
    auto it = _ins_map.find(ins[0]);
    assert(it != _ins_map.end());
    auto infos = it->second;
    assert(infos->kind == Kind::BRANCH);

    std::vector<std::size_t> res;
    for (std::size_t i = 0; i < infos->args.size(); ++i)
      if (infos->args[i] == ArgKind::TARGET)
        res.push_back(i);
    return res;
  }

private:
  Kind kind;
  std::string name;
  std::size_t nargs_min;
  std::size_t nargs_max;
  std::vector<ArgKind> args;

  static std::vector<std::size_t> _list_regs(const def_t &ins,
                                             std::size_t begin = 0) {
    std::vector<std::size_t> res;
    for (std::size_t i = begin; i < ins.size(); ++i)
      if (is_reg(ins[i]))
        res.push_back(i);
    return res;
  }

  std::string _check_call(const def_t &ins) const {
    if (ins.size() < 2)
      return FMT_OSS("Invalid number of arguments");
    bool has_ret = ins[1][0] == '%';
    if (has_ret && !is_reg(ins[1]))
      return FMT_OSS("Expected def register, got `" << ins[1] << "'");
    if (!is_label(ins[1 + has_ret]))
      return FMT_OSS("Expected function label, got `" << ins[1 + has_ret]
                                                      << "'");

    for (std::size_t i = 2 + has_ret; i < ins.size(); ++i)
      if (!is_val(ins[i]))
        return FMT_OSS("Expected function argument value, got `" << ins[i]
                                                                 << "'");

    return "";
  }

  std::string _check_ret(const def_t &ins) const {
    if (ins.size() > 2)
      return FMT_OSS("Cannot have multiple return values");
    if (ins.size() == 2 && !is_val(ins[1]))
      return FMT_OSS("Expected function return value, got `" << ins[1] << "'");

    return "";
  }

  std::string _check_phi(const def_t &ins) const {
    if (ins.size() < 4 || ins.size() % 2 != 0)
      return FMT_OSS("Invalid number of arguments");

    for (std::size_t i = 2; i < ins.size(); i += 2) {
      if (!is_label(ins[i]))
        return FMT_OSS("Expected label, got `" << ins[i] << "'");
      if (!is_val(ins[i + 1]))
        return FMT_OSS("Expected value, got `" << ins[i + 1] << "'");
    }

    return "";
  }

  std::string _check(const def_t &ins) const {
    assert(ins[0] == name);

    if (ins.size() < nargs_min || ins.size() > nargs_max)
      return FMT_OSS("Invalid number of arguments");

    for (std::size_t i = 0; i < args.size(); ++i) {
      auto kind = args[i];
      const auto &arg = ins[i];
      if (kind == ArgKind::DEF && !is_reg(arg))
        return FMT_OSS("Invalid argument: expected a register, got `" << arg
                                                                      << "'");
      else if (kind == ArgKind::VAL && !is_val(arg))
        return FMT_OSS("Invalid argument: expected a value, got `" << arg
                                                                   << "'");
      else if (kind == ArgKind::TARGET && !is_label(arg))
        return FMT_OSS("Invalid argument: expected a label, got `" << arg
                                                                   << "'");
    }

    if (ins[0] == "call")
      return _check_call(ins);
    if (ins[0] == "ret")
      return _check_ret(ins);
    if (ins[0] == "phi")
      return _check_phi(ins);

    return "";
  }
};

std::map<std::string, InsInfos *> InsInfos::_ins_map{};

const InsInfos ilist[] = {
    "add d v v", "@b t",        "@bc v t t", "@beq v v t t",
    "call *",    "cmplt d v v", "mov d v",   "mul d v v",
    "phi d *",   "@ret *",      "sub d v v",
};

std::ostream &operator<<(std::ostream &os, const def_t &args) {
  dump(os, args);
  return os;
}

} // namespace

std::size_t def_idx(const def_t &ins) { return InsInfos::def_idx(ins); }

std::string get_def(const def_t &ins) {
  auto idx = def_idx(ins);
  assert(idx != IDX_NO);
  return ins[idx].substr(1);
}

std::vector<std::size_t> uses_idxs(const def_t &ins) {
  return InsInfos::uses_idxs(ins);
}

std::set<std::string> get_uses(const def_t &ins) {
  std::set<std::string> res;
  for (auto idx : uses_idxs(ins))
    res.insert(ins[idx].substr(1));
  return res;
}

std::set<std::string> get_regs(const def_t &ins) {
  auto res = get_uses(ins);
  if (def_idx(ins) != IDX_NO)
    res.insert(get_def(ins));
  return res;
}

std::vector<std::string> get_labels(const def_t &ins) {
  std::vector<std::string> res;
  for (const auto &arg : ins)
    if (is_label(arg))
      res.push_back(arg.substr(1));
  return res;
}

void replace_regs(def_t &ins, const std::string &old_regs,
                  const std::string &new_regs) {
  std::string vold = "%" + old_regs;
  std::string vnew = "%" + new_regs;
  for (auto &arg : ins)
    if (arg == vold)
      arg = vnew;
}

void replace_labels(def_t &ins, const std::string &old_label,
                    const std::string &new_label) {
  std::string vold = "@" + old_label;
  std::string vnew = "@" + new_label;
  for (auto &arg : ins)
    if (arg == vold)
      arg = vnew;
}

bool is_branch(const def_t &ins) { return InsInfos::is_branch(ins); }

std::vector<std::size_t> branch_targets_idxs(const def_t &ins) {
  return InsInfos::targets_idxs(ins);
}

std::vector<std::string> branch_targets(const def_t &ins) {
  std::vector<std::string> res;
  for (auto idx : InsInfos::targets_idxs(ins))
    res.push_back(ins[idx].substr(1));
  return res;
}

bool is_call(const def_t &ins) { return ins[0] == "call"; }

bool call_has_ret(const def_t &ins) { return ins[1][0] == '%'; }

std::string call_get_function_name(const def_t &ins) {
  if (call_has_ret(ins))
    return ins[2].substr(1);
  else
    return ins[1].substr(1);
}

std::size_t call_args_beg(const def_t &ins) { return 2 + call_has_ret(ins); }

std::size_t call_args_end(const def_t &ins) { return ins.size(); }

std::size_t call_args_count(const def_t &ins) {
  return call_args_end(ins) - call_args_beg(ins);
}

std::size_t phi_find_label(const def_t &ins, const std::string &label) {
  assert(ins[0] == "phi");
  for (std::size_t i = 2; i < ins.size(); i += 2)
    if (ins[i].substr(1) == label)
      return i;
  return IDX_NO;
}

std::vector<std::string> phi_get_labels(const def_t &ins) {
  std::vector<std::string> res;
  for (std::size_t i = 2; i < ins.size(); i += 2)
    res.push_back(ins[i].substr(1));
  return res;
}

bool fundecl_has_ret(const def_t &dir) { return dir[1] != "void"; }

std::size_t fundecl_args_beg(const def_t &dir) {
  (void)dir;
  return 2;
}

std::size_t fundecl_args_end(const def_t &dir) { return dir.size(); }

std::size_t fundecl_args_count(const def_t &dir) {
  return fundecl_args_end(dir) - fundecl_args_beg(dir);
}

std::vector<std::string> fundecl_args(const def_t &dir) {
  std::vector<std::string> res;
  for (std::size_t i = fundecl_args_beg(dir); i < fundecl_args_end(dir); ++i)
    res.push_back(dir[i].substr(1));
  return res;
}

void fundecl_rename_arg(def_t &dir, std::size_t idx,
                        const std::string &new_arg) {
  assert(idx < fundecl_args_count(dir));
  dir[fundecl_args_beg(dir) + idx] = '%' + new_arg;
}

void dump(std::ostream &os, const def_t &ins) {
  for (std::size_t i = 0; i < ins.size(); ++i) {
    os << ins[i];
    if (i + 1 < ins.size())
      os << (i == 0 ? " " : ", ");
  }
}

void check_ins(const def_t &ins) {
  auto err = InsInfos::check(ins);
  PANIC_IF(!err.empty(), FMT_OSS(err << " at `" << ins << "'"));
}

void check(const gop::Module &mod) {

#define LC_ERR(Mess) PANIC(FMT_OSS(Mess))

  // prototypes of all defined functions
  std::map<std::string, def_t> _funs;
  std::vector<std::size_t> _funs_pos;

  // Build prototypes and ranges for all functions
  for (std::size_t i = 0; i < mod.decls.size(); ++i) {
    auto dir = dynamic_cast<gop::Dir *>(mod.decls[i].get());

    if (dir && dir->args[0] == "fun") {
      if (dir->label_defs.size() != 1)
        LC_ERR("fun directive `" << dir->args << "' requires one label");

      const auto &name = dir->label_defs[0];
      if (!_funs.emplace(name, dir->args).second)
        LC_ERR("Redefinition of `" << name << "' at `" << dir->args << "'");

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
    std::set<std::string> defs;
    for (const auto &r : fundecl_args(fun_dir->args))
      defs.insert(r);

    for (auto it = beg; it != end; ++it) {
      auto ins = dynamic_cast<gop::Ins *>(it->get());

      if (ins) {
        for (auto &lbl : ins->label_defs)
          if (!labels.insert(lbl).second)
            LC_ERR("Multiple definition of label `" << lbl << "' at `"
                                                    << ins->args << "'");

        auto err = InsInfos::check(ins->args);
        if (!err.empty())
          LC_ERR(err << " at `" << ins->args << "'");

        if (def_idx(ins->args) != IDX_NO)
          defs.insert(get_def(ins->args));
      }
    }

    // Second pass to check all instructions
    for (auto it = beg; it != end; ++it) {
      auto ins = dynamic_cast<gop::Ins *>(it->get());
      auto dir = dynamic_cast<gop::Dir *>(it->get());
      if (dir)
        LC_ERR("Unexpected directive at `" << dir->args << "'");

      if (is_branch(ins->args)) {
        for (const auto &t : branch_targets(ins->args))
          if (!labels.count(t))
            LC_ERR("Branching to undefined label `" << t << "' at `"
                                                    << ins->args << "'");
      }

      for (const auto &r : get_uses(ins->args))
        if (!defs.count(r))
          LC_ERR("Use of undefined register `" << r << "' at `" << ins->args
                                               << "'");

      if (is_call(ins->args)) {
        auto it = _funs.find(call_get_function_name(ins->args));
        if (it != _funs.end()) { // Call to undefined function is not an error

          const auto &callee_decl = it->second;
          if (call_args_count(ins->args) != fundecl_args_count(callee_decl) ||
              call_has_ret(ins->args) != fundecl_has_ret(callee_decl))
            LC_ERR("Call instruction doesn't match declaration `"
                   << callee_decl << "' at `" << ins->args << "'");
        }
      }

      else if (ins->args[0] == "ret") {
        if ((ins->args.size() > 1) != fundecl_has_ret(fun_dir->args))
          LC_ERR("Return instruction doesn't match declaration `"
                 << fun_dir->args << "' at `" << ins->args << "'");
      }

      else if (ins->args[0] == "phi") {
        // Partial check of phi, difficult to know if all preds are listed, and
        // if every pred is valid.
        // there is no BBs anymore in this representation
        for (const auto &lbl : phi_get_labels(ins->args))
          if (!labels.count(lbl))
            LC_ERR("Usage of undefined label `" << lbl << "' at `" << ins->args
                                                << "'");
      }
    }
  }

#undef LC_ERR
}

} // namespace isa
