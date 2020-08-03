#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "fwd.hh"

namespace gop {

// generic declaration in ASM file
struct Decl {
  // All labels defined right before the declaration
  std::vector<std::string> label_defs;

  // All full-line comments right before the declaration
  std::vector<std::string> comm_pre;

  // Optional end-of-line command, enpty if none
  std::string comm_eol;

  Decl(const std::vector<std::string> &label_defs = {},
       const std::vector<std::string> &comm_pre = {},
       const std::string &comm_eol = {})
      : label_defs(label_defs), comm_pre(comm_pre), comm_eol(comm_eol) {}

  Decl(const Decl &) = delete;

  virtual ~Decl() = default;

  // Only dump 1-line text, without comments or label defs
  virtual void dump(std::ostream &os) const = 0;
};

// generic instruction
struct Ins : public Decl {
  std::vector<std::string> args;

  Ins(const std::vector<std::string> &args) : Decl(), args(args) {}

  static std::unique_ptr<Ins> parse(const std::string &str);

  void dump(std::ostream &os) const override;
};

// generic directive
struct Dir : public Decl {
  std::vector<std::string> args;

  Dir(const std::vector<std::string> &args) : Decl(), args(args) {}

  static std::unique_ptr<Dir> parse(const std::string &str);

  void dump(std::ostream &os) const override;
};

struct Module {
  std::vector<std::unique_ptr<Decl>> decls;

  Module() = default;
  Module(const Module &) = delete;
  Module(Module &&) = default;

  Module clone() const;

  static Module parse(std::istream &is);

  void dump(std::ostream &os) const;
};

} // namespace gop
