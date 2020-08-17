#pragma once

#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include <gop10/fwd.hh>

namespace isa {

class Context;

enum class InsKind {
  CALL,
  RET,
  BRANCH,
  NORMAL,
};

using arg_kind_t = unsigned;
constexpr arg_kind_t ARG_KIND_NONE = 0;
constexpr arg_kind_t ARG_KIND_REG = 1;
constexpr arg_kind_t ARG_KIND_CONST = 2;
constexpr arg_kind_t ARG_KIND_BLOCK = 4;
constexpr arg_kind_t ARG_KIND_FUN = 8;
constexpr arg_kind_t ARG_KIND_USE = 16;
constexpr arg_kind_t ARG_KIND_DEF = 32;
constexpr arg_kind_t ARG_KIND_USEDEF = 64;
constexpr arg_kind_t ARG_KIND_REPEAT = 128;

// Syntax:
// <kind> <opname> <arg>*
// <kind>: call | ret | branch | normal
// <arg>:
// r: reg
// c: const
// b: block
// f: fun
// u: use
// d: def
// x: use + def
// *: repeat (variadic)
// Can combine many: u|c (use register or constant)
//   a|b means arg can be a or b (not that it's both)
//   eg u|d means arg can be a use, or can be a def
//     use x to means it's both
//
struct InsInfos {

  InsInfos(Context &ctx, const std::string &fmt);

  Context &ctx() const { return _ctx; }
  InsKind kind() const { return _kind; }
  const std::string &opname() const { return _opname; }
  const std::vector<arg_kind_t> &args() const { return _args; }
  arg_kind_t get_arg(std::size_t pos) const;
  bool has_def() const;

  bool is_variadic() const;

private:
  Context &_ctx;
  InsKind _kind;
  std::string _opname;
  std::vector<arg_kind_t> _args;
};

struct RegInfos {
  std::string name;

  RegInfos(const std::string &name) : name(name) {}
};

// Represent all infos about a specific ISA
// (Can use many ISAs at once
//
// Syntax:
// <ins-infos>\n
// <ins-infos>\n
// ...
class Context {

public:
  Context(const std::string &isa_path);

  bool is_instruction(const std::string &opname) const;
  const InsInfos &get_infos(const std::string &opname) const;

  // Special regs always defined
  void add_reg(const std::string &name);
  std::vector<const RegInfos *> regs() const;
  const RegInfos &get_reg(const std::string &name) const;

  const std::vector<std::unique_ptr<InsInfos>> &all_ins() const {
    return _ins_infos;
  }

private:
  std::vector<std::unique_ptr<InsInfos>> _ins_infos;
  std::map<std::string, const InsInfos *> _infos_map;

  std::vector<std::unique_ptr<RegInfos>> _regs;
  std::map<std::string, const RegInfos *> _regs_map;
};

class Ins {

public:
  Ins(const Context &ctx, const std::vector<std::string> &args)
      : _ctx(ctx), _args(args) {}

  const Context &ctx() const { return _ctx; }
  const std::vector<std::string> &args() const { return _args; }

  const InsInfos &infos() const;
  InsKind kind() const;
  bool is_term() const; // last block instruction
  const std::string &opname() const { return _args[0]; }
  arg_kind_t get_arg_kind(std::size_t idx) const;

  // Check if a syntax is valid (syntax only)
  void check() const;

  // List of blocks the instruction may jump too
  std::vector<std::string> target_blocks() const;

  // Name of registers uses arguments
  std::vector<std::string> args_uses() const;

  // Name of registers defs arguments
  std::vector<std::string> args_defs() const;

private:
  const Context &_ctx;
  const std::vector<std::string> &_args;
};

std::ostream &operator<<(std::ostream &os, const Ins &ins);

// Check if a module is valid
// Panic if any error is found
void check(const Context &ctx, const gop::Module &mod);

} // namespace isa
