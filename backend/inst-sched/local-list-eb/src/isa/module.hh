#pragma once

#include <memory>
#include <ostream>

#include "analysis.hh"
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

  template <class T> const T &get_analysis() const;

  template <class T> void invalidate_analysis();

private:
  std::string _name;
  std::vector<std::string> _args;
  std::vector<std::unique_ptr<BasicBlock>> _blocks;
  Module *_parent;

  mutable std::vector<std::unique_ptr<FunctionAnalysis>> _analysises;
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

template <class T> const T &Function::get_analysis() const {
  for (const auto &fa : _analysises)
    if (auto res = dynamic_cast<const T *>(fa.get()))
      return *res;

  auto new_fa = std::make_unique<T>(*this);
  const T &res = *new_fa;
  _analysises.push_back(std::move(new_fa));
  return res;
}

template <class T> void Function::invalidate_analysis() {
  auto it = _analysises.begin();
  while (it != _analysises.end() && !dynamic_cast<T *>(it->get()))
    ++it;

  if (it != _analysises.end())
    _analysises.erase(it);
}

} // namespace isa
