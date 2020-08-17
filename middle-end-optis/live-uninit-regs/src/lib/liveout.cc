#include "liveout.hh"

#include <fstream>
#include <map>
#include <sstream>

#include "cfg.hh"

namespace {

bool is_reg(const std::string &str) { return str.size() > 1 && str[0] == '%'; }

regs_set_t filter_regs(const regs_set_t &regs) {
  regs_set_t res;
  for (const auto &r : regs)
    if (is_reg(r))
      res.insert(r);
  return res;
}

// @return src registers of instruction (read from)
// eg: uses(x <- a + b) = {a, b}
regs_set_t ins_uses(const Ins &ins) {
  if (ins.args[0] == "add")
    return filter_regs({ins.args[2], ins.args[3]});
  else if (ins.args[0] == "beq")
    return filter_regs({ins.args[1], ins.args[2]});
  else if (ins.args[0] == "mov")
    return filter_regs({ins.args[2]});
  else if (ins.args[0] == "print")
    return filter_regs({ins.args[1]});
  else
    return {};
}

// @return dst registers of instruction (written to)
// eg: defs(x <- a + b) = {x}
regs_set_t ins_defs(const Ins &ins) {
  if (ins.args[0] == "add")
    return {ins.args[1]};
  else if (ins.args[0] == "mov")
    return {ins.args[1]};
  else
    return {};
}

// Compute the uevar and varkill sets associated with a BasicBlock
// uevar: Upward Exposed vars: variables used in bb before any redefiniton in
// bb
// varkill: Variables defined in bb (kill the previous definition)
void compute_uevar_varkill(const BasicBlock &bb, regs_set_t &uevar,
                           regs_set_t &varkill) {
  uevar.clear();
  varkill.clear();

  for (const auto &ins : bb.ins()) {

    // Insert all uses not-defined yet in uevar
    for (const auto &r : ins_uses(ins))
      if (varkill.count(r) == 0)
        uevar.insert(r);

    // Insert all defs in varkill
    for (const auto &r : ins_defs(ins))
      varkill.insert(r);
  }
}

void dump_str(const std::string &str, std::size_t len = 0) {
  if (str.size() < len)
    for (std::size_t i = 0; i < len - str.size(); ++i)
      std::cout << ' ';
  std::cout << str;
}

void dump_regs_set(const regs_set_t &regs, std::size_t len = 0) {
  std::ostringstream os;
  os << '{';
  bool first = true;
  for (const auto &it : regs) {
    if (!first)
      os << ", ";
    first = false;
    os << it;
  }
  os << '}';
  dump_str(os.str(), len);
}

class LiveOut {
public:
  liveout_res_t run(const IModule &mod) {
    _mod = &mod;

    // Step 1: build CFG
    _cfg = std::make_unique<Digraph>(build_cfg(mod));
    std::ofstream ofs("out.dot");
    _cfg->dump_tree(ofs);

    // Step 2: Compute Uevars / Varkill set of all BBs
    for (auto bb : mod.bb_list()) {
      auto &uevar = _bbs_uevars[bb];
      auto &varkill = _bbs_varkills[bb];
      compute_uevar_varkill(*bb, uevar, varkill);
    }
    _dump_uevar_varkills();

    // Step 3: Update liveout until they don't change anymore
    std::size_t iter = 0;
    for (;;) {
      bool changed = false;
      if (iter == 0)
        changed = _init_all_liveouts();

      changed = _update_all_liveout();

      if (!changed)
        break;
      _dump_liveouts(iter);
      ++iter;
    }

    return _bbs_liveout;
  }

private:
  const IModule *_mod;
  std::unique_ptr<Digraph> _cfg;
  std::map<const BasicBlock *, regs_set_t> _bbs_uevars;
  std::map<const BasicBlock *, regs_set_t> _bbs_varkills;
  liveout_res_t _bbs_liveout;

  // Start initializing the livout of bb using only UEVar of successors
  // Needed only once because UEVar doesn't change
  // @returns true if liveout[bb] changed
  bool _init_liveout(const BasicBlock *bb) {
    regs_set_t &liveout = _bbs_liveout[bb];
    liveout.clear();
    bool changed = false;

    for (auto it = _cfg->adj_begin(bb->id()); it != _cfg->adj_end(bb->id());
         ++it) {
      const BasicBlock *m = _mod->get_bb(*it);
      assert(m);
      const regs_set_t &uevar = _bbs_uevars[m];
      changed |= !uevar.empty();
      liveout.insert(uevar.begin(), uevar.end());
    }

    return changed;
  }

  // Initalize the liveout of all basic blocks in module
  // @returns true if liveout changed
  bool _init_all_liveouts() {
    bool changed = false;
    for (auto bb : _mod->bb_list())
      if (_init_liveout(bb))
        changed = true;
    return changed;
  }

  // Update the liveout of bb using equation
  // LiveOut(bb) = |_{m in Succs(bb)} (UEVar(m) | (LiveOut(m) & ~VarKill(m)))
  // Doesn't need to recompute everything every time because:
  // - UeVar, Varkill didn't change, only LiveOut(m) was updated since previous
  // iteration
  // - LiveOut(m) can only grow
  // @returns true if liveout bb[changed]
  bool _update_liveout(const BasicBlock *bb) {
    regs_set_t &liveout = _bbs_liveout[bb];
    bool changed = false;

    for (auto it = _cfg->adj_begin(bb->id()); it != _cfg->adj_end(bb->id());
         ++it) {
      const BasicBlock *m = _mod->get_bb(*it);
      const regs_set_t &m_varkill = _bbs_varkills[m];
      const regs_set_t &m_liveout = _bbs_liveout[m];

      for (const auto &r : m_liveout)
        if (liveout.count(r) == 0 && m_varkill.count(r) == 0) {
          // inserted if in ~Varkill(m), and  not already in LiveOut(bb)
          liveout.insert(r);
          changed = true;
        }
    }

    return changed;
  }

  // Update liveout of all basic blocks
  // @returns true if liveout changed
  bool _update_all_liveout() {
    bool changed = false;
    for (auto bb : _mod->bb_list())
      if (_update_liveout(bb))
        changed = true;
    return changed;
  }

  void _dump_uevar_varkills() const {

    dump_str("label", 10);
    dump_str("UEVar", 30);
    dump_str("VarKill", 30);
    std::cout << "\n";

    for (const auto &it : _bbs_uevars) {
      auto bb = it.first;
      dump_str(bb->label(), 10);
      dump_regs_set(_bbs_uevars.find(bb)->second, 30);
      dump_regs_set(_bbs_varkills.find(bb)->second, 30);
      std::cout << "\n";
    }
    std::cout << "\n";
  }

  void _dump_liveouts(std::size_t iter) const {
    std::cout << "Liveout (Iteration #" << (iter + 1) << "):\n";
    for (auto bb : _mod->bb_list()) {
      dump_str(bb->label(), 10);
      dump_regs_set(_bbs_liveout.find(bb)->second, 30);
      std::cout << "\n";
    }
    std::cout << "\n";
  }
};

} // namespace

liveout_res_t run_liveout(const IModule &mod) {
  LiveOut lo;
  return lo.run(mod);
}
