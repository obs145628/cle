#include "tree-block.hh"

#include <cassert>
#include <fstream>

#include "../utils/digraph.hh"
#include <utils/str/format-string.hh>

namespace {

std::size_t get_pred(const std::string &arg, isa::arg_kind_t kind,
                     std::string &def, std::size_t ins_idx,
                     const std::map<std::string, TreeBlock::Ins *> &ins_list,
                     std::vector<std::unique_ptr<TreeBlock::Node>> &nodes) {
  if (kind == isa::ARG_KIND_DEF) {
    assert(def.empty());
    def = arg.substr(1);
    return ID_NONE;
  }

  if (kind == isa::ARG_KIND_BLOCK || kind == isa::ARG_KIND_FUN)
    return ID_NONE;

  if (kind == isa::ARG_KIND_CONST) {
    auto node = std::make_unique<TreeBlock::Const>();
    auto id = nodes.size();
    node->idx = id;
    node->parent = ins_idx;
    node->val = std::atol(arg.c_str());
    nodes.push_back(std::move(node));
    return id;
  }

  if (kind == isa::ARG_KIND_USE) {
    auto name = arg.substr(1);
    auto it = ins_list.find(name);
    if (it != ins_list.end()) {
      auto ins = it->second;
      assert(ins->parent == ID_NONE);
      ins->parent = ins_idx;
      return ins->idx;
    }

    auto node = std::make_unique<TreeBlock::Reg>();
    auto id = nodes.size();
    node->idx = id;
    node->parent = ins_idx;
    node->reg = name;
    nodes.push_back(std::move(node));
    return id;
  }

  assert(0);
}

std::string node_label(const TreeBlock::Node &node) {
  std::string res = std::to_string(node.idx) + ": ";
  if (auto ins = dynamic_cast<const TreeBlock::Ins *>(&node))
    return FMT_OSS(node.idx << ": Ins(" << ins->ins.opname() << ")");
  else if (auto c = dynamic_cast<const TreeBlock::Const *>(&node))
    return FMT_OSS(node.idx << ": Const(" << c->val << ")");
  else if (auto r = dynamic_cast<const TreeBlock::Reg *>(&node))
    return FMT_OSS(node.idx << ": Reg(" << r->reg << ")");
  assert(0);
}

} // namespace

TreeBlock::TreeBlock(const isa::Context &ctx, const gop::Module &mod) {

  std::map<std::string, Ins *> ins_list;

  for (const auto &decl : mod.decls) {
    auto ins = dynamic_cast<gop::Ins *>(decl.get());
    if (!ins)
      continue;

    isa::Ins cins(ctx, ins->args);
    _nodes.push_back(std::make_unique<Ins>(cins));
    auto node = dynamic_cast<Ins *>(_nodes.back().get());
    node->idx = _nodes.size() - 1;
    node->parent = ID_NONE;

    std::string def;
    for (std::size_t i = 1; i < ins->args.size(); ++i) {
      auto idx = get_pred(ins->args[i], cins.get_arg_kind(i - 1), def,
                          node->idx, ins_list, _nodes);
      node->children.push_back(idx);
    }

    if (!def.empty())
      ins_list.emplace(def, node);
  }

  // Find root
  _root = ID_NONE;
  for (const auto &node : _nodes)
    if (node->parent == ID_NONE) {
      assert(_root == ID_NONE);
      _root = node->idx;
    }
  assert(_root != ID_NONE);

  dump_tree("ir_block.dot");
}

void TreeBlock::dump_tree(const std::string &out_path) const {
  Digraph g(_nodes.size());
  for (const auto &node : _nodes) {
    g.labels_set_vertex_name(node->idx, node_label(*node));
    if (node->parent != ID_NONE)
      g.add_edge(node->parent, node->idx);
  }

  std::ofstream os(out_path);
  g.dump_tree(os);
}
