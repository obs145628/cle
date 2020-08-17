#include "lcm.hh"

#include <iostream>
#include <map>
#include <set>

#include "cfg.hh"

namespace {

bool is_reg(const std::string &str) { return str.size() > 1 && str[0] == '%'; }

bool is_def(const Ins &ins) { return is_reg(ins.args.back()); }

std::string get_def(const Ins &ins) {
  if (is_def(ins))
    return ins.args.back().substr(1);
  else
    return "";
}

std::set<std::string> get_uses(const Ins &ins) {
  std::set<std::string> res;
  for (std::size_t i = 1; i + 1 < ins.args.size(); ++i)
    if (is_reg(ins.args[i]))
      res.insert(ins.args[i].substr(1));
  return res;
}

std::set<std::string> set_or(const std::set<std::string> &a,
                             const std::set<std::string> &b) {
  std::set<std::string> res = a;
  res.insert(b.begin(), b.end());
  return res;
}

std::set<std::string> set_and(const std::set<std::string> &a,
                              const std::set<std::string> &b) {
  std::set<std::string> res;
  for (const auto &x : a)
    if (b.count(x))
      res.insert(x);
  return res;
}

std::set<std::string> set_nand(const std::set<std::string> &a,
                               const std::set<std::string> &b) {
  std::set<std::string> res;
  for (const auto &x : a)
    if (!b.count(x))
      res.insert(x);
  return res;
}

class LCM {
public:
  LCM(IModule &mod) : _mod(mod), _cfg(build_cfg(_mod)) {}

  void run() {
    // Compute static data needed
    _list_exprs();
    for (auto bb : _mod.bb_list())
      _build_bb_sets(bb);
    std::cout << "\n";

    // Compute all control flow informations
    _compute_avail_in();
    _compute_ant_out();
    _compute_earlier();
    _compute_later_in();

    // Compute which expr should be inserted / deleted where in code
    _compute_ins_del();

    // Update code
    _rewrite();
  }

private:
  IModule &_mod;
  Digraph _cfg;

  struct Def {
    bool is_expr;
    std::string def;
    std::set<std::string> uses;
    std::set<std::string> users;
  };

  std::map<std::string, Def> _defs;
  std::map<BasicBlock *, std::set<std::string>> _deexpr;
  std::map<BasicBlock *, std::set<std::string>> _ueexpr;
  std::map<BasicBlock *, std::set<std::string>> _exprkill;

  std::map<BasicBlock *, std::set<std::string>> _avail_in;
  std::map<BasicBlock *, std::set<std::string>> _avail_out;
  std::map<BasicBlock *, std::set<std::string>> _ant_in;
  std::map<BasicBlock *, std::set<std::string>> _ant_out;

  std::vector<std::set<std::string>> _earlier;
  std::map<BasicBlock *, std::set<std::string>> _later_in;
  std::vector<std::set<std::string>> _later;

  std::vector<std::set<std::string>> _insert;
  std::map<BasicBlock *, std::set<std::string>> _delete;

  Def &_get_def_first(const std::string &name) {
    if (!_defs.count(name)) {
      Def new_def;
      new_def.def = name;
      new_def.is_expr = false;
      _defs.emplace(name, new_def);
    }
    return _defs.at(name);
  }

  // Generate list of all expressions in the module
  void _list_exprs() {
    for (auto bb : _mod.bb_list())
      for (auto ins : bb->ins()) {

        auto def_name = get_def(ins);
        auto uses = get_uses(ins);
        bool is_expr = ins.args[0] != "i2i";
        _get_def_first(def_name);

        if (!def_name.empty() && is_expr) {
          Def &def = _get_def_first(def_name);
          assert(!def.is_expr);
          def.is_expr = true;
          def.uses = uses;
        }

        if (!def_name.empty())
          for (const auto &u : uses) {
            Def &def = _get_def_first(u);
            def.users.insert(def_name);
          }
      }

    std::cout << "Exprs: {";
    for (const auto &it : _defs)
      if (it.second.is_expr)
        std::cout << it.second.def << "; ";
    std::cout << "}\n";
  }

  // Downard-Exposed Exprs: e in DEExpr(bb) iff bb evaluates e and none of e
  // operands defined between last eval of e and and end of bb
  // Upward-Exposed Exprs: e in UEExpr(bb) iff bb use e before any of its
  // operands is redefined in bb
  // ExprKill: e in ExprKill(bb) iff any operand of e is redefined  in bb
  void _build_bb_sets(BasicBlock *bb) {
    auto &deexpr = _deexpr[bb];
    auto &ueexpr = _ueexpr[bb];
    auto &exprkill = _exprkill[bb];

    // First pass to compute DEExpr / Exprkill
    for (auto it = bb->ins().rbegin(); it != bb->ins().rend(); ++it) {
      auto def_name = get_def(*it);
      Def *def = def_name.empty() ? nullptr : &_defs.at(def_name);

      if (def && !def->is_expr) ////only definition matters
        for (const auto &u : def->users)
          if (_defs.at(u).is_expr)
            exprkill.insert(u);

      if (def && def->is_expr &&
          !exprkill.count(def_name)) // expr e is evaluated and e not killed in
                                     // any of the next instructions
        deexpr.insert(def_name);
    }

    // Second pas to compute UEExpr / Exprkill (again)
    exprkill.clear();
    for (const auto &ins : bb->ins()) {
      auto def_name = get_def(ins);
      Def *def = def_name.empty() ? nullptr : &_defs.at(def_name);

      // I am not sure which of the 2 is it
      /*
      for (const auto &u : get_uses(ins))
        if (_defs.at(u).is_expr &&
            !exprkill.count(u)) // expr e is used and e not killed
                                // in any of the previous instructions
          ueexpr.insert(u);
      */

      if (def && def->is_expr &&
          !exprkill.count(def_name)) // expr e is evaluated and e not killed in
                                     // any of the previous instructions
        ueexpr.insert(def_name);

      // I am not sure either if exprkill udp should be above or below uuexpr
      // Below makes more sense
      // But Above makes code right for example
      if (def && !def->is_expr) ////only definition matters
        for (const auto &u : def->users)
          if (_defs.at(u).is_expr)
            exprkill.insert(u);
    }

    std::cout << bb->label() << ": ";
    std::cout << "DEExpr: {";
    for (const auto &e : deexpr)
      std::cout << e << "; ";
    std::cout << "}, ";
    std::cout << "UEExpr: {";
    for (const auto &e : ueexpr)
      std::cout << e << "; ";
    std::cout << "}, ";
    std::cout << "ExprKill: {";
    for (const auto &e : exprkill)
      std::cout << e << "; ";
    std::cout << "}\n";
  }

  // expression e is Available at point p iff in every path from entry to p e is
  // evaluated and none of its operands are redefined
  //
  // Init:
  // AvailIn(b0) = {}
  // AvailIn(b) = {all e}, b != b0
  //
  // Recursion:
  // AvailIn(n) = &_{m in preds(n)} (DEExpr(m) | (AvailIn(m) & ~ExprKill(m)))
  //
  // AvailOut(m) = DEExpr(m) | (AvailIn(m) & ~ExprKill(m))
  void _compute_avail_in() {
    // Initialization
    for (auto bb : _mod.bb_list()) {
      if (bb != &_mod.get_entry_bb())
        _avail_in[bb] = _all_exprs();
      else
        _avail_in[bb] = {};
    }

    bool changed = true;
    int niter = 0;
    std::cout << "AvailIn:\n";

    while (changed) {
      changed = _step_avail_in();

      std::cout << "Step #" << ++niter << ": ";
      for (auto bb : _mod.bb_list()) {
        std::cout << bb->label() << ": {";
        for (const auto &e : _avail_in.at(bb))
          std::cout << e << "; ";
        std::cout << "}, ";
      }
      std::cout << "\n";
    }

    // Compute AvailOut from AvailIn
    for (auto &m : _mod.bb_list())
      _avail_out[m] = set_or(_deexpr[m], set_nand(_avail_in[m], _exprkill[m]));

    std::cout << "AvailOut:\n";
    for (auto bb : _mod.bb_list()) {
      std::cout << bb->label() << ": {";
      for (const auto &e : _avail_out.at(bb))
        std::cout << e << "; ";
      std::cout << "}, ";
    }
    std::cout << "\n\n";
  }

  bool _step_avail_in() {
    bool changed = false;

    for (auto n : _mod.bb_list()) {

      auto &old_set = _avail_in[n];
      std::set<std::string> new_set = _all_exprs();
      auto preds = _preds(*n);
      if (preds.empty())
        continue;

      for (auto m : preds) {
        auto m_set = set_or(_deexpr[m], set_nand(_avail_in[m], _exprkill[m]));
        new_set = set_and(new_set, m_set);
      }

      if (new_set != old_set) {
        old_set = new_set;
        changed = true;
      }
    }

    return changed;
  }

  // expression e is Anticipable at point p iff in every path from p to exit e
  // is evaluated before any of its operands is redefined
  //
  // Init:
  // AntOut(bf) = {}
  // AntOut(b) = {all e}, b != bf
  //
  // Recursion:
  // AntOut(n) = &_{m in succs(n)} (UEExpr(m) | (AntOut(m) & ~ExprKill(m)))
  //
  // AntIn(m) = UEExpr(m) | (AntOut(m) & ~ExprKill(m))
  void _compute_ant_out() {
    // Initialization
    for (auto bb : _mod.bb_list()) {
      if (_succs(*bb).size())
        _ant_out[bb] = _all_exprs();
      else
        _ant_out[bb] = {};
    }

    bool changed = true;
    int niter = 0;
    std::cout << "AntOut:\n";

    while (changed) {
      changed = _step_ant_out();

      std::cout << "Step #" << ++niter << ": ";
      for (auto bb : _mod.bb_list()) {
        std::cout << bb->label() << ": {";
        for (const auto &e : _ant_out.at(bb))
          std::cout << e << "; ";
        std::cout << "}, ";
      }
      std::cout << "\n";
    }

    // Compute AntIn from AntOut
    for (auto &m : _mod.bb_list())
      _ant_in[m] = set_or(_ueexpr[m], set_nand(_ant_out[m], _exprkill[m]));

    std::cout << "AntIn:\n";
    for (auto bb : _mod.bb_list()) {
      std::cout << bb->label() << ": {";
      for (const auto &e : _ant_in.at(bb))
        std::cout << e << "; ";
      std::cout << "}, ";
    }
    std::cout << "\n\n";
  }

  bool _step_ant_out() {
    bool changed = false;

    for (auto n : _mod.bb_list()) {

      auto &old_set = _ant_out[n];
      std::set<std::string> new_set = _all_exprs();
      auto succs = _succs(*n);
      if (succs.empty())
        continue;

      for (auto m : succs) {
        auto m_set = set_or(_ueexpr[m], set_nand(_ant_out[m], _exprkill[m]));
        new_set = set_and(new_set, m_set);
      }

      if (new_set != old_set) {
        old_set = new_set;
        changed = true;
      }
    }

    return changed;
  }

  void _compute_earlier() {
    _earlier.resize(_mod.bb_count() * _mod.bb_count());

    for (auto i : _mod.bb_list())
      for (auto j : _succs(*i)) {
        auto &earl = _get_earlier(*i, *j);
        earl = set_nand(_ant_in[j], _avail_out[i]);

        if (i != &_mod.get_entry_bb()) {
          std::set<std::string> filter;
          for (const auto &x : earl)
            if (!_exprkill[i].count(x) && _ant_out[i].count(x))
              filter.insert(x);
          for (const auto &x : filter)
            earl.erase(x);
        }

        std::cout << "Earliest(" << i->label() << ", " << j->label() << "): {";
        for (const auto &e : earl)
          std::cout << e << "; ";
        std::cout << "}\n";
      }

    std::cout << "\n";
  }

  void _compute_later_in() {
    // Initialization
    for (auto bb : _mod.bb_list()) {
      if (bb != &_mod.get_entry_bb())
        _later_in[bb] = _all_exprs();
      else
        _later_in[bb] = {};
    }

    bool changed = true;
    int niter = 0;
    std::cout << "LaternIn:\n";

    while (changed) {
      changed = _step_later_in();

      std::cout << "Step #" << ++niter << ": ";
      for (auto bb : _mod.bb_list()) {
        std::cout << bb->label() << ": {";
        for (const auto &e : _later_in.at(bb))
          std::cout << e << "; ";
        std::cout << "}, ";
      }
      std::cout << "\n";
    }

    // Compute Later from LaterIn
    _later.resize(_mod.bb_count() * _mod.bb_count());
    for (auto i : _mod.bb_list())
      for (auto j : _succs(*i)) {
        auto &later = _get_later(*i, *j);
        later =
            set_or(_get_earlier(*i, *j), set_nand(_later_in[i], _ueexpr[i]));

        std::cout << "Later(" << i->label() << ", " << j->label() << "): {";
        for (const auto &e : later)
          std::cout << e << "; ";
        std::cout << "}\n";
      }
    std::cout << "\n";
  }

  bool _step_later_in() {
    bool changed = false;

    for (auto j : _mod.bb_list()) {

      auto &old_set = _later_in[j];
      std::set<std::string> new_set = _all_exprs();
      auto preds = _preds(*j);
      if (preds.empty())
        continue;

      for (auto i : preds) {
        auto i_set =
            set_or(_get_earlier(*i, *j), set_nand(_later_in[i], _ueexpr[i]));
        new_set = set_and(new_set, i_set);
      }

      if (new_set != old_set) {
        old_set = new_set;
        changed = true;
      }
    }

    return changed;
  }

  void _compute_ins_del() {
    _insert.resize(_mod.bb_count() * _mod.bb_count());
    for (auto i : _mod.bb_list()) {
      if (i == &_mod.get_entry_bb())
        _delete[i] = {};
      else
        _delete[i] = set_nand(_ueexpr[i], _later_in[i]);

      for (auto j : _succs(*i)) {
        _get_insert(*i, *j) = set_nand(_get_later(*i, *j), _later_in[j]);
        std::cout << "Insert(" << i->label() << ", " << j->label() << "): {";
        for (const auto &e : _get_insert(*i, *j))
          std::cout << e << "; ";
        std::cout << "}\n";
      }
    }

    for (auto i : _mod.bb_list()) {
      std::cout << "Delete(" << i->label() << "): {";
      for (const auto &e : _delete[i])
        std::cout << e << "; ";
      std::cout << "}\n";
    }
    std::cout << "\n";
  }

  void _rewrite() {

    std::map<std::string, std::vector<std::string>> move_code;

    for (auto bb : _mod.bb_list()) {

      auto del = _delete.at(bb);
      for (std::size_t i = 0; i < bb->ins().size(); ++i) {
        const auto &ins = bb->ins()[i];
        auto def_name = get_def(ins);
        if (del.count(def_name)) {
          move_code.emplace(def_name, ins.args);
          bb->ins().erase(bb->ins().begin() + i--);
        }
      }
    }

    for (auto i : _mod.bb_list())
      for (auto j : _succs(*i)) {
        const auto &ins = _get_insert(*i, *j);
        if (ins.empty())
          continue;

        std::vector<std::vector<std::string>> code;
        for (const auto &x : ins)
          code.push_back(move_code.at(x));

        if (_succs(*i).size() == 1)
          _insert_in(*i, code, /*at_end=*/true);
        else if (_preds(*j).size() == 1)
          _insert_in(*j, code, /*at_end=*/false);
        else
          _insert_between(*i, *j, code);
      }
  }

  void _insert_between(BasicBlock &i, BasicBlock &j,
                       const std::vector<std::vector<std::string>> &move_code) {
    // Create new basic block with code
    auto &mid = _mod.add_bb();
    _insert_in(mid, move_code, /*at_end=*/true);

    // Add jump to j at end of new bb
    Ins term;
    term.args = {"b", "@" + j.label()};
    mid.ins().push_back(term);

    // Replace jump to j from i by a jump to the new bb
    auto &br_i = i.ins().back().args;
    for (auto &x : br_i)
      if (x == "@" + j.label())
        x = "@" + mid.label();
  }

  void _insert_in(BasicBlock &bb,
                  const std::vector<std::vector<std::string>> &move_code,
                  bool at_end) {
    std::vector<Ins> new_code;
    for (auto x : move_code) {
      Ins ins;
      ins.args = x;
      new_code.push_back(ins);
    }

    if (at_end)
      bb.ins().insert(at_end ? bb.ins().end() : bb.ins().begin(),
                      new_code.begin(), new_code.end());
  }

  std::set<std::string> &_get_earlier(BasicBlock &i, BasicBlock &j) {
    return _earlier[i.id() * _mod.bb_count() + j.id()];
  }

  std::set<std::string> &_get_later(BasicBlock &i, BasicBlock &j) {
    return _later[i.id() * _mod.bb_count() + j.id()];
  }

  std::set<std::string> &_get_insert(BasicBlock &i, BasicBlock &j) {
    return _insert[i.id() * _mod.bb_count() + j.id()];
  }

  std::vector<BasicBlock *> _preds(BasicBlock &bb) {
    std::vector<BasicBlock *> res;
    for (auto v : _cfg.get_preds(bb.id()))
      res.push_back(_mod.get_bb(v));
    return res;
  }

  std::vector<BasicBlock *> _succs(BasicBlock &bb) {
    std::vector<BasicBlock *> res;
    for (auto v : _cfg.get_succs(bb.id()))
      res.push_back(_mod.get_bb(v));
    return res;
  }

  std::set<std::string> _all_exprs() const {
    std::set<std::string> res;
    for (const auto &d : _defs)
      if (d.second.is_expr)
        res.insert(d.second.def);
    return res;
  }
};

} // namespace

void run_lcm(IModule &mod) {
  LCM lcm(mod);
  lcm.run();
}
