#pragma once

#include <memory>
#include <ostream>

#include "isa.hh"
#include <gop10/module.hh>
#include <logia/fwd.hh>

namespace isa {

class BasicBlock;
class Function;
class Module;

class BasicBlock {
public:
  using ins_t = std::vector<std::string>;

  BasicBlock(const std::string &name, const std::vector<ins_t> &code,
             Function *parent)
      : _name(name), _code(code), _parent(parent) {}

  const std::string &name() const { return _name; }
  std::vector<ins_t> &code() { return _code; }
  const std::vector<ins_t> &code() const { return _code; }
  const Function &parent() const { return *_parent; }

  void add_ins(const ins_t &ins);

  void dump_code(std::ostream &os) const;
  void dump_code(logia::MdGfmDoc &doc) const;
  void dump_ins(std::ostream &os, std::size_t idx) const;

  void check() const;

private:
  std::string _name;
  std::vector<ins_t> _code;
  Function *_parent;
};

class Function {
public:
  Function(const std::string &name, const std::vector<std::string> &args,
           Module *parent)
      : _name(name), _args(args), _parent(parent) {}

  const std::string &name() const { return _name; }
  const std::vector<std::string> &args() const { return _args; }
  const Module &parent() const { return *_parent; }

  std::vector<BasicBlock *> bbs();
  std::vector<const BasicBlock *> bbs() const;

  BasicBlock *add_block(const std::string &name);

  void dump_code(std::ostream &os) const;
  void dump_code(logia::MdGfmDoc &doc) const;

  void check() const;

private:
  std::string _name;
  std::vector<std::string> _args;
  std::vector<std::unique_ptr<BasicBlock>> _blocks;
  Module *_parent;
};

class Module {

public:
  Module(const isa::Context &ctx);
  Module(const isa::Context &ctx, const gop::Module &mod);

  const isa::Context &ctx() const { return _ctx; }

  std::vector<Function *> funs();
  std::vector<const Function *> funs() const;

  Function *add_fun(const std::string &name,
                    const std::vector<std::string> &args);

  void dump_code(std::ostream &os) const;
  void dump_code(logia::MdGfmDoc &doc) const;

  void check() const;

private:
  const isa::Context &_ctx;
  std::vector<std::unique_ptr<Function>> _funs;
};

} // namespace isa
