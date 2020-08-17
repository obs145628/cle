#include "loader.hh"

#include <fstream>

#include "../isa/isa.hh"
#include <utils/cli/err.hh>

namespace {

class ModuleBuilder {

public:
  ModuleBuilder(const gop::Module &mod) : _mod(mod) {}

  std::unique_ptr<Module> run() {
    isa::check(_mod);
    _res = Module::create();
    PANIC_IF(_mod.decls.size() == 0, "empty code");

    // First pass: build instructions and use special value for ins / bb / fun
    // uses

    BasicBlock *next_bb = nullptr;
    Function *next_fun = nullptr;

    // Make list of all bbs (divide by branch instructions)
    for (const auto &dec : _mod.decls) {

      auto dir = dynamic_cast<const gop::Dir *>(dec.get());
      if (dir && dir->args[0] == "fun") {
        PANIC_IF(dir->label_defs.size() != 1, "Missing function name");
        if (next_fun)
          _fix(*next_fun);
        next_fun = &_res->add_fun(dir->label_defs[0], dir->args);
        _fun_map.emplace(next_fun->get_name(), next_fun);
        _ruse = ValueConst::make(46);
        _entry = nullptr;
        continue;
      }

      auto ins = dynamic_cast<const gop::Ins *>(dec.get());
      assert(ins);
      PANIC_IF(!next_fun, "Code outside of any function");

      if (next_bb == nullptr) {
        auto label = ins->label_defs.size() > 0 ? ins->label_defs[0] : "";
        next_bb = &next_fun->add_bb(label);
        _bb_map.emplace(next_bb->get_name(), next_bb);
        if (!next_fun->has_entry_bb()) {
          next_fun->set_entry_bb(*next_bb);
          _entry = next_bb;
        }
      }

      auto ins_it = parse_ins(next_bb, ins->args);
      _ins_map.emplace(&*ins_it, ins);
      // ins_it->dump(std::cout);
      // std::cout << "\n";
      if (ins_it->is_branch()) // end of basic block
        next_bb = nullptr;
    }

    PANIC_IF(next_bb != nullptr, "Last instruction in module isn't a branch");
    assert(next_fun);
    _fix(*next_fun);

    // Check module is well formed
    _res->check();
    return std::move(_res);
  }

private:
  const gop::Module &_mod;
  std::unique_ptr<Module> _res;
  std::map<Instruction *, const gop::Ins *> _ins_map;
  std::map<std::string, Value *> _def_map;
  std::map<std::string, Value *> _bb_map;
  std::map<std::string, Value *> _fun_map;

  Value *_ruse;
  BasicBlock *_entry;

  ins_iterator_t parse_ins(BasicBlock *bb,
                           const std::vector<std::string> &args) {
    auto ins = args[0];
    auto def_idx = isa::def_idx(args);
    std::string name;
    if (def_idx != isa::IDX_NO)
      name = isa::get_def(args);

    std::vector<Value *> ops;
    for (std::size_t i = 1; i < args.size(); ++i)
      if (i != def_idx)
        ops.push_back(_parse_arg(args[i]));

    auto it = bb->insert_ins(bb->ins_end(), ins, ops, name, def_idx);
    if (!name.empty())
      _def_map.emplace(name, &*it);
    return it;
  }

  Value *_parse_arg(const std::string &val) {
    if (val[0] == '%') {
      assert(_ruse);
      return _ruse;
    } else if (val[0] == '@') {
      assert(_entry);
      return _entry;
    } else {
      return ValueConst::make(std::atol(val.c_str()));
    }
  }

  // Replace all special labels / regs in fun by their true value
  void _fix(Function &fun) {

    auto args = isa::fundecl_args(fun.decl());
    for (std::size_t i = 0; i < args.size(); ++i) {
      _def_map.emplace(args[i], &fun.get_arg(i));
    }

    for (auto &bb : fun.bb())
      for (auto &ins : bb.ins()) {
        auto it = _ins_map.find(&ins);
        assert(it != _ins_map.end());
        _fix(ins, it->second->args);
      }

    _ins_map.clear();
    _def_map.clear();
    _bb_map.clear();
  }

  void _fix(Instruction &ins, const std::vector<std::string> &args) {
    Module &mod = *_res;

    auto def_idx = isa::def_idx(args);
    std::size_t j = 0;
    for (std::size_t i = 1; i < args.size(); ++i) {
      const auto &arg = args[i];
      if (i == def_idx)
        continue;

      // Label
      if (arg[0] == '@') {
        auto bb_it = _bb_map.find(arg.substr(1));
        auto tbb = bb_it == _bb_map.end() ? nullptr : bb_it->second;
        auto fun_it = _fun_map.find(arg.substr(1));
        auto tfun = fun_it == _fun_map.end() ? nullptr : fun_it->second;
        if (tbb)
          ins.set_op(j++, *tbb);
        else if (tfun)
          ins.set_op(j++, *tfun);
        else {
          auto &new_fun = mod.add_fun(arg.substr(1), {".fun", "void"}, true);
          ins.set_op(j++, new_fun);
          _fun_map.emplace(new_fun.get_name(), &new_fun);
        }
      }

      // Register
      else if (arg[0] == '%') {
        auto it = _def_map.find(arg.substr(1));
        PANIC_IF(it == _def_map.end(), "Undefined register use");
        ins.set_op(j++, *it->second);
      }

      else {
        ++j;
      }
    }
  }
};

} // namespace

std::unique_ptr<Module> load_module(const gop::Module &mod) {
  ModuleBuilder mb(mod);
  return mb.run();
}

std::unique_ptr<Module> load_module(std::istream &is) {
  auto mod = gop::Module::parse(is);
  return load_module(mod);
}

std::unique_ptr<Module> load_module(const std::string &path) {
  std::ifstream is(path);
  return load_module(is);
}

gop::Module mod2gop(const Module &mod) {
  gop::Module res;

  for (auto &f : mod.fun()) {
    if (!f.has_def())
      continue;

    auto decl = f.decl();
    for (std::size_t i = 0; i < f.args_count(); ++i)
      isa::fundecl_rename_arg(decl, i, f.get_arg(i).get_name());
    auto gfun = std::make_unique<gop::Dir>(decl);
    gfun->label_defs = {f.get_name()};
    res.decls.push_back(std::move(gfun));

    for (const auto &bb : f.bb()) {
      bool is_first = true;
      for (const auto &ins : bb.ins()) {

        auto gins = std::make_unique<gop::Ins>(ins.sargs());
        if (is_first)
          gins->label_defs = {bb.get_name()};

        res.decls.push_back(std::move(gins));
        is_first = false;
      }
    }
  }

  return res;
}
