#include "cfg.hh"

#include <map>

#include "../isa/isa.hh"
#include <logia/md-gfm-doc.hh>
#include <logia/program.hh>

namespace {

Digraph build_cfg(const isa::Function &fun,
                  const VertexAdapter<const isa::BasicBlock *> &va) {
  Digraph g(va.size());
  std::map<std::string, const isa::BasicBlock *> bb_names;
  for (const auto &bb : fun.bbs())
    bb_names.emplace(bb->name(), bb);

  for (auto bb : fun.bbs()) {
    g.labels_set_vertex_name(va(bb), bb->name());
    isa::Ins bins(fun.parent().ctx(), bb->code().back());
    if (bins.kind() == isa::InsKind::BRANCH)
      for (const auto &succ : bins.target_blocks())
        g.add_edge(va(bb), va(bb_names.at(succ)));
  }

  return g;
}

} // namespace

CFG::CFG(const isa::Function &fun)
    : FunctionAnalysis(fun), _va(fun.bbs()), _g(build_cfg(fun, _va)) {

  auto doc = logia::Program::instance().add_doc<logia::MdGfmDoc>("CFG: @" +
                                                                 fun.name());
  _g.dump_tree(*doc);
}

std::vector<const isa::BasicBlock *>
CFG::preds(const isa::BasicBlock &bb) const {
  std::vector<const isa::BasicBlock *> res;
  for (auto p : _g.preds(_va(&bb)))
    res.push_back(_va(p));
  return res;
}

std::vector<const isa::BasicBlock *>
CFG::succs(const isa::BasicBlock &bb) const {
  std::vector<const isa::BasicBlock *> res;
  for (auto p : _g.succs(_va(&bb)))
    res.push_back(_va(p));
  return res;
}
