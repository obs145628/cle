#include "module.hh"

#include <cassert>

#include <logia/md-gfm-doc.hh>
#include <utils/cli/err.hh>

namespace isa {

void BasicBlock::add_ins(const ins_t &ins) { _code.push_back(ins); }

void BasicBlock::dump_code(std::ostream &os) const {
  os << _name << ":\n";
  for (std::size_t i = 0; i < _code.size(); ++i) {
    dump_ins(os, i);
    os << "\n";
  }
}

void BasicBlock::dump_code(logia::MdGfmDoc &doc) const {
  auto ch = doc.code("asm");
  dump_code(ch.os());
}

void BasicBlock::dump_ins(std::ostream &os, std::size_t idx) const {
  os << "    ";
  const auto &ins = _code.at(idx);
  for (std::size_t i = 0; i < ins.size(); ++i) {
    if (i == 1)
      os << " ";
    else if (i > 1)
      os << ", ";
    os << ins[i];
  }
}

void BasicBlock::check() const {
  for (const auto &ins : _code) {
    isa::Ins cins(parent().parent().ctx(), ins);
    cins.check();
  }
}

BasicBlock *Function::add_block(const std::string &name) {
  auto block = std::make_unique<BasicBlock>(
      name, std::vector<BasicBlock::ins_t>{}, this);
  auto res = block.get();
  _blocks.push_back(std::move(block));
  return res;
}

std::vector<BasicBlock *> Function::bbs() {
  std::vector<BasicBlock *> res;
  for (const auto &bb : _blocks)
    res.push_back(bb.get());
  return res;
}

std::vector<const BasicBlock *> Function::bbs() const {
  std::vector<const BasicBlock *> res;
  for (const auto &bb : _blocks)
    res.push_back(bb.get());
  return res;
}

void Function::dump_code(std::ostream &os) const {
  os << _name << ":\n";
  os << ".fun ";
  for (std::size_t i = 0; i < args().size(); ++i) {
    if (i > 0)
      os << ", ";
    os << "%" << _args[i];
  }
  os << "\n\n";

  for (const auto &bb : _blocks) {
    bb->dump_code(os);
    os << "\n";
  }
}

void Function::dump_code(logia::MdGfmDoc &doc) const {
  auto ch = doc.code("asm");
  dump_code(ch.os());
}

void Function::check() const {
  for (const auto &bb : _blocks)
    bb->check();
}

Module::Module(const isa::Context &ctx) : _ctx(ctx) {}

Module::Module(const isa::Context &ctx, const gop::Module &mod) : Module(ctx) {

  Function *fun = nullptr;
  BasicBlock *bb = nullptr;

  for (auto &decl : mod.decls) {

    if (auto dir = dynamic_cast<const gop::Dir *>(decl.get())) {
      assert(dir->label_defs.size() == 1);
      const auto &name = dir->label_defs.front();
      std::vector<std::string> args;
      for (std::size_t i = 2; i < dir->args.size(); ++i)
        args.push_back(dir->args[i].substr(1));
      fun = add_fun(name, args);
      continue;
    }

    const auto &ins = dynamic_cast<const gop::Ins &>(*decl);
    if (ins.label_defs.size() > 0) {
      assert(ins.label_defs.size() == 1);
      PANIC_IF(bb, "label in the middle of a block");
      PANIC_IF(!fun, "block outside of a function");
      bb = fun->add_block(ins.label_defs.front());
    }

    PANIC_IF(!bb, "instruction outside of a block");
    bb->add_ins(ins.args);

    isa::Ins cins(_ctx, ins.args);
    if (cins.is_term())
      bb = nullptr;
  }

  PANIC_IF(bb, "unfinished block");
}

std::vector<Function *> Module::funs() {
  std::vector<Function *> res;
  for (auto &f : _funs)
    res.push_back(f.get());
  return res;
}

std::vector<const Function *> Module::funs() const {
  std::vector<const Function *> res;
  for (const auto &f : _funs)
    res.push_back(f.get());
  return res;
}

Function *Module::add_fun(const std::string &name,
                          const std::vector<std::string> &args) {
  auto fun = std::make_unique<Function>(name, args, this);
  auto res = fun.get();
  _funs.push_back(std::move(fun));
  return res;
}

void Module::dump_code(std::ostream &os) const {
  for (const auto &f : _funs) {
    f->dump_code(os);
    os << "\n";
  }
}

void Module::dump_code(logia::MdGfmDoc &doc) const {
  auto ch = doc.code("asm");
  dump_code(ch.os());
}

void Module::check() const {
  for (const auto &fun : _funs)
    fun->check();
}

} // namespace isa
