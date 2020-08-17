#include "module-load.hh"

#include <fstream>

#include "isa.hh"
#include <utils/cli/err.hh>

std::unique_ptr<Module> load_module(const gop::Module &mod,
                                    std::vector<CallInfos> &cc_freqs) {
  auto res = Module::create();
  PANIC_IF(mod.decls.size() == 0, "empty code");

  BasicBlock *next_bb = nullptr;
  Function *next_fun = nullptr;

  // Make list of all bbs (divide by branch instructions)
  for (const auto &dec : mod.decls) {

    auto dir = dynamic_cast<const gop::Dir *>(dec.get());
    if (dir) {
      PANIC_IF(dir->name != "fun", "Unknown directive");
      PANIC_IF(dir->label_defs.size() != 1, "Missing function name");
      next_fun = &res->add_fun(dir->label_defs[0], dir->args);
      continue;
    }

    auto ins = dynamic_cast<const gop::Ins *>(dec.get());
    assert(ins);
    PANIC_IF(!next_fun, "Code outside of any function");

    if (ins->args[0] == "call") {
      CallInfos ci;
      ci.src = next_fun->name();
      ci.dst = ins->args[1];
      ci.val = std::atoi(ins->comm_eol.c_str());
      cc_freqs.push_back(ci);
    }

    if (next_bb == nullptr) {
      auto label = ins->label_defs.size() > 0 ? ins->label_defs[0] : "";
      next_bb = &next_fun->add_bb(label);
      if (!next_fun->has_entry_bb())
        next_fun->set_entry_bb(*next_bb);
    }

    auto ins_it = next_bb->insert_ins(next_bb->ins_end(), ins->args);
    if (isa::is_branch(*ins_it)) // end of basic block
      next_bb = nullptr;
  }

  // Check module is well formed
  PANIC_IF(next_bb != nullptr, "Last instruction in module isn't a branch");
  res->check();
  return res;
}

std::unique_ptr<Module> load_module(std::istream &is,
                                    std::vector<CallInfos> &cc_freqs) {
  auto mod = gop::Module::parse(is);
  return load_module(mod, cc_freqs);
}

std::unique_ptr<Module> load_module(const std::string &path,
                                    std::vector<CallInfos> &cc_freqs) {
  std::ifstream is(path);
  return load_module(is, cc_freqs);
}

gop::Module mod2gop(const Module &mod) {
  gop::Module res;
  for (auto &f : mod.fun()) {
    auto gfun = std::make_unique<gop::Dir>("fun", f.args());
    gfun->label_defs = {f.name()};
    res.decls.push_back(std::move(gfun));

    for (const auto &bb : f.bb()) {
      bool is_first = true;
      for (const auto &ins : bb.ins()) {

        auto gins = std::make_unique<gop::Ins>(ins.args);
        if (is_first)
          gins->label_defs = {bb.label()};

        res.decls.push_back(std::move(gins));
        is_first = false;
      }
    }
  }

  return res;
}
