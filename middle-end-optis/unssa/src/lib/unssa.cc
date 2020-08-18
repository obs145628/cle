#include "unssa.hh"

#include <fstream>
#include <iostream>

#include "../isa/isa.hh"
#include "cfg.hh"
#include "critical.hh"
#include "digraph-order.hh"
#include "digraph.hh"
#include "module.hh"
#include "vertex-adapter.hh"

namespace {

class UnSSA {

public:
  UnSSA(const Function &fun, gop::Module &res)
      : _fun(fun), _res(res), _cfg(_fun) {}

  void run() {

    // Insert extra code to replace phis
    for (const auto &bb : _fun.bb())
      for (const auto &ins : bb.ins()) {
        if (ins.get_opname() != "phi")
          break;

        for (std::size_t i = 0; i < ins.ops_count(); i += 2) {
          auto pred = dynamic_cast<const BasicBlock *>(&ins.op(i));
          assert(pred);
          assert(!_is_crit(*pred, bb));
          _extra[pred].push_back(
              {"mov", "%" + ins.get_name(), ins.op(i + 1).to_arg()});
        }
      }

    // Schedule inserted code
    for (const auto &bb : _fun.bb())
      _schedule(bb);

    // Write whole function code
    _write_code();
  }

private:
  const Function &_fun;
  gop::Module &_res;
  CFG _cfg;
  std::map<const BasicBlock *, std::vector<isa::ins_t>> _extra;

  bool _is_crit(const BasicBlock &src, const BasicBlock &dst) {
    return _cfg.succs(src).size() > 1 && _cfg.preds(dst).size() > 1;
  }

  // Write asm code
  // Almost exaclt like in loader.cc
  // Expect phi are ignored
  // And some optional code is added at the end of some bb (right before
  // branching)
  void _write_code() {

    auto decl = _fun.decl();
    for (std::size_t i = 0; i < _fun.args_count(); ++i)
      isa::fundecl_rename_arg(decl, i, _fun.get_arg(i).get_name());
    auto gfun = std::make_unique<gop::Dir>(decl);
    gfun->label_defs = {_fun.get_name()};
    _res.decls.push_back(std::move(gfun));

    for (const auto &bb : _fun.bb()) {
      bool is_first = true;
      const auto &extra = _extra[&bb];

      for (const auto &ins : bb.ins()) {
        if (ins.get_opname() == "phi")
          continue;

        if (ins.is_branch() && !extra.empty()) {
          // Add extra instructions to replace PHI
          for (const auto &args : extra)
            _add_ins(bb, args, is_first);
        }

        _add_ins(bb, ins.sargs(), is_first);
      }
    }
  }

  class CycleSolve {
  public:
    CycleSolve(Digraph &g, VertexAdapter<std::string> &va,
               std::vector<isa::ins_t> &new_code)
        : _g(g), _va(va), _new_code(new_code) {}

    void run() {
      _on_stack.assign(_g.v(), 0);
      _marked.assign(_g.v(), 0);

      for (std::size_t i = 0; i < _g.v(); ++i) {
        if (!_marked[i])
          _dfs(i);
      }
    }

  private:
    Digraph &_g;
    VertexAdapter<std::string> &_va;
    std::vector<isa::ins_t> &_new_code;

    std::vector<int> _on_stack;
    std::vector<int> _marked;

    void _dfs(std::size_t v) {
      assert(!_marked[v]);
      assert(!_on_stack[v]);
      _marked[v] = 1;
      _on_stack[v] = 1;

      for (auto w : _g.succs(v)) {

        if (!_marked[w])
          _dfs(w);

        else if (_on_stack[w]) {
          // Cycle detected
          // replace all edges u -> w  by u -> px forall u
          // And add copy operation executed before all scheduled operations:
          // mov px, w
          std::cout << "Cycle detected at mov " << _va(v) << ", " << _va(w)
                    << "\n";

          auto px = _va("%p" + std::to_string(_new_code.size()));
          std::vector<std::size_t> preds;
          for (auto u : _g.preds(w))
            preds.push_back(u);
          for (auto u : preds) {
            _g.del_edge(u, w);
            _g.add_edge(u, px);
          }

          _new_code.push_back({"mov", _va(px), _va(w)});
        }
      }

      _on_stack[v] = 0;
    }
  };

  void _schedule(const BasicBlock &bb) {
    std::vector<isa::ins_t> &code = _extra[&bb];
    if (code.size() <= 1)
      return;

    // Make list of values
    std::set<std::string> vals;
    for (const auto &ins : code) {
      vals.insert(ins[1]);
      vals.insert(ins[2]);
    }
    // Add extra temp vars (optionally used to break cycles)
    std::size_t nb_temps = vals.size();
    for (std::size_t i = 0; i < nb_temps; ++i)
      vals.insert("%p" + std::to_string(i));
    VertexAdapter<std::string> va(vals);

    // Build scheduling graph
    Digraph g(va.size());
    for (std::size_t i = 0; i < va.size(); ++i)
      g.labels_set_vertex_name(i, va(i));
    for (auto &ins : code) {
      g.add_edge(va(ins[1]), va(ins[2]));
    }

    // Verify graph is well formed
    for (std::size_t i = 0; i < va.size(); ++i)
      assert(g.out_deg(i) <= 1);

    std::ofstream fos1("./dg_" + bb.get_name() + ".dot");
    g.dump_tree(fos1);

    // Insert copy operations to remove cycles
    std::vector<isa::ins_t> new_code;
    CycleSolve cs(g, va, new_code);
    cs.run();
    std::ofstream fos2("./dag_" + bb.get_name() + ".dot");
    g.dump_tree(fos2);

    // Schedule moves
    // Use topological sort (reverse post order)
    // This make sure a mov (pred) is done before it's value is rewritten (succ)
    //  (all preds of a node must be visited before being visited)
    auto topo = digraph_dfs(g, DFSOrder::REV_POST);
    for (auto v : topo) {
      if (!g.out_deg(v))
        continue;
      auto dst = va(v);
      auto src = va(g.succs(v).front());
      new_code.push_back({"mov", dst, src});
    }

    code = new_code;
  }

  void _add_ins(const BasicBlock &bb, const isa::ins_t &ins, bool &is_first) {
    auto gins = std::make_unique<gop::Ins>(ins);
    if (is_first)
      gins->label_defs = {bb.get_name()};

    _res.decls.push_back(std::move(gins));
    is_first = false;
  }
};

} // namespace

gop::Module unssa(const Module &mod) {

  gop::Module res;

  for (auto &fun : mod.fun()) {
    if (!fun.has_def())
      continue;
    UnSSA us(fun, res);
    us.run();
  }

  return res;
}
