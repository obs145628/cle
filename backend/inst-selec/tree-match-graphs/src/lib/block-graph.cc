#include "block-graph.hh"

#include <algorithm>
#include <cassert>
#include <set>

#include "../utils/digraph.hh"
#include "../utils/vertex-adapter.hh"
#include <logia/md-gfm-doc.hh>
#include <logia/program.hh>
#include <utils/str/format-string.hh>

namespace {
std::vector<isa::arg_kind_t> extract_args(const isa::Ins &ins) {
  std::vector<isa::arg_kind_t> args;
  for (std::size_t i = 0; i < ins.args().size() - 1; ++i)
    args.push_back(ins.get_arg_kind(i));
  return args;
}

std::string extract_def(const isa::Ins &ins) {
  for (std::size_t i = 0; i < ins.args().size() - 1; ++i)
    if (ins.get_arg_kind(i) == isa::ARG_KIND_DEF)
      return ins.args()[i + 1].substr(1);
  return "";
}

} // namespace

BlockGraph::Node::Node(const std::vector<Node *> &succs) : succs(succs) {
  for (auto s : succs)
    if (s)
      s->preds.push_back(this);
}

std::size_t BlockGraph::Node::depth() const {
  if (succs.empty())
    return 0;

  std::size_t max_child = 0;
  for (auto s : succs)
    if (s)
      max_child = std::max(max_child, s->depth());
  return 1 + max_child;
}

void BlockGraph::Node::replace_succ(Node &old_succ, Node &new_succ) {
  assert(&old_succ != &new_succ);
  auto it = std::find(succs.begin(), succs.end(), &old_succ);
  assert(it != succs.end());

  auto old_it = std::find(old_succ.preds.begin(), old_succ.preds.end(), this);
  assert(old_it != succs.end());
  old_succ.preds.erase(old_it);

  new_succ.preds.push_back(this);
  *it = &new_succ;
}

BlockGraph::Ins::Ins(const std::vector<Node *> &succs, const isa::Ins ins,
                     int order)
    : Ins(succs, ins.opname(), extract_args(ins), extract_def(ins), order) {
  assert(succs.size() + 1 == ins.args().size());
}

BlockGraph::Ins::Ins(const std::vector<Node *> &succs,
                     const std::string &opname,
                     const std::vector<isa::arg_kind_t> &args,
                     const std::string &def, int order)
    : Node(succs), opname(opname), args(args), def(def), order(order) {
  assert(args.size() == succs.size());
}

BlockGraph::BlockGraph(const BasicBlock &bb)
    : _bb(bb), _ctx(_bb.parent().parent().ctx()),
      _live_out(bb.parent().get_analysis<LiveOut>()) {
  _build();

  auto doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(FMT_OSS(
      "Initial BlockGraph for @" << _bb.parent().name() << ":@" << _bb.name()));
  _bb.dump_code(*doc);
  to_graph().dump_tree(*doc);
}

std::vector<const BlockGraph::Node *> BlockGraph::nodes_list() const {
  std::vector<const BlockGraph::Node *> res;
  for (const auto &x : _nodes)
    res.push_back(x.get());
  return res;
}

Digraph BlockGraph::to_graph() const {
  VertexAdapter<const BlockGraph::Node *> va(nodes_list());
  Digraph g(count());

  for (const auto &node : _nodes) {
    g.labels_set_vertex_name(va(node.get()), _node_label(*node));
    for (auto s : node->succs)
      if (s)
        g.add_edge(va(node.get()), va(s));
  }

  return g;
}

Digraph BlockGraph::to_graph(const Ins &root) const {
  auto nodes = get_nodes_in(root);
  VertexAdapter<const BlockGraph::Node *> va(nodes);
  Digraph g(nodes.size());

  for (auto node : nodes) {
    g.labels_set_vertex_name(va(node), _node_label(*node));
    for (auto s : node->succs)
      if (s)
        g.add_edge(va(node), va(s));
  }

  return g;
}

void BlockGraph::to_trees() {
  // Turn graph into a forest of trees
  // Find nodes with more than 1 pred
  // Node can be either:
  // - extracted, value stored in reg, then used by its old preds
  // - duplicated (need to duplicate all nodes in the subtree for each pred)

  for (;;) {
    Node *node = nullptr;
    for (auto &n : _nodes)
      if (n->preds.size() > 1) {
        node = n.get();
        break;
      }

    if (!node)
      break;

    _extract_node(*node);
  }

  auto doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(FMT_OSS(
      "Forest BlockGraph for @" << _bb.parent().name() << ":@" << _bb.name()));
  _bb.dump_code(*doc);
  to_graph().dump_tree(*doc);
}

std::vector<const BlockGraph::Ins *> BlockGraph::get_roots() const {
  std::vector<const Ins *> res;
  for (const auto &n : _nodes)
    if (n->preds.size() == 0) {
      const auto &ins = dynamic_cast<const Ins &>(*n);
      res.push_back(&ins);
    }
  return res;
}

std::vector<const BlockGraph::Node *>
BlockGraph::get_nodes_in(const Ins &root) const {
  std::vector<const Node *> res;
  assert(root.preds.empty());

  std::vector<const Node *> st;
  st.push_back(&root);

  while (!st.empty()) {
    auto node = st.back();
    st.pop_back();
    res.push_back(node);
    for (auto s : node->succs)
      if (s)
        st.push_back(s);
  }

  return res;
}

void BlockGraph::_build() {

  std::size_t order = 0;

  // 1 : Add all regs defined outside of this block
  // (Only add ones used in this block to reduce list)

  // Compute regs used in this block
  std::set<std::string> this_uses;
  for (const auto &ins : _bb.code()) {
    isa::Ins cins(_ctx, ins);
    for (const auto &r : cins.args_uses())
      this_uses.insert(r);
  }

  // Add special ISA regs to reg list
  for (const auto &reg : _ctx.regs())
    if (this_uses.count(reg->name))
      _add_extern_reg(reg->name);

  // Add function arguments to reg list
  for (const auto &arg : _bb.parent().args())
    if (this_uses.count(arg))
      _add_extern_reg(arg);

  // Add registers defined in other blocks
  for (const auto &r : _get_extern_block_regs())
    if (this_uses.count(r))
      _add_extern_reg(r);

  for (const auto &ins : _bb.code()) {
    isa::Ins cins(_ctx, ins);
    auto node = _build_ins(cins, order++);
    if (!node->def.empty())
      _regs_nodes[node->def] = node; // replace if multiple defs
  }
}

BlockGraph::Node *BlockGraph::_build_arg(const isa::Ins &ins, std::size_t idx) {
  const std::string &arg = ins.args()[idx + 1];
  auto kind = ins.get_arg_kind(idx);
  if (kind == isa::ARG_KIND_DEF) // def is no argument
    return nullptr;

  // not implemented
  assert(kind != isa::ARG_KIND_FUN && kind != isa::ARG_KIND_USEDEF &&
         kind != isa::ARG_KIND_REG);

  if (kind == isa::ARG_KIND_BLOCK)
    return &_add_block(arg.substr(1));

  if (kind == isa::ARG_KIND_CONST)
    return &_add_const(std::atol(arg.c_str()));

  if (kind == isa::ARG_KIND_USE) {
    auto name = arg.substr(1);
    auto *arg_node = _regs_nodes.at(name);
    if (dynamic_cast<const BlockGraph::Ins *>(arg_node) &&
        _live_out.liveout(_bb).count(name)) {
      // Computed value is live, must always be stored in its register
      return &_add_reg(name);
    }
    return arg_node;
  }

  assert(0);
}

BlockGraph::Ins *BlockGraph::_build_ins(const isa::Ins &ins,
                                        std::size_t order) {

  std::vector<Node *> args;
  for (std::size_t i = 0; i < ins.args().size() - 1; ++i)
    args.push_back(_build_arg(ins, i));

  auto node = std::make_unique<Ins>(args, ins, order);
  auto res = node.get();
  _nodes.push_back(std::move(node));
  return res;
}

std::vector<std::string> BlockGraph::_get_extern_block_regs() {
  // Get all registers defined in another block
  std::vector<std::string> res;

  for (auto bb : _bb.parent().bbs()) {
    if (bb == &_bb)
      continue;

    for (const auto &ins : bb->code()) {
      isa::Ins cins(_ctx, ins);
      for (const auto &r : cins.args_defs())
        if (std::find(res.begin(), res.end(), r) == res.end())
          res.push_back(r);
    }
  }

  return res;
}

void BlockGraph::_extract_node(Node &node) {
  auto preds = node.preds;

  // If block, const, or reg, just clone the node
  if (auto b = dynamic_cast<BlockRef *>(&node)) {
    for (std::size_t i = 1; i < preds.size(); ++i) {
      auto &new_node = _add_block(b->name);
      preds[i]->replace_succ(node, new_node);
    }
    return;
  }

  if (auto c = dynamic_cast<Const *>(&node)) {
    for (std::size_t i = 1; i < preds.size(); ++i) {
      auto &new_node = _add_const(c->val);
      preds[i]->replace_succ(node, new_node);
    }
    return;
  }

  if (auto r = dynamic_cast<Reg *>(&node)) {
    for (std::size_t i = 1; i < preds.size(); ++i) {
      auto &new_node = _add_reg(r->reg);
      preds[i]->replace_succ(node, new_node);
    }
    return;
  }

  // For ins, clone computation graph, or just replace with a reference to the
  // reg, Depending on ins depth
  auto &ins = dynamic_cast<Ins &>(node);
  if (ins.depth() < 2)
    _clone_ins(ins);
  else
    _extract_ins(ins);
}

void BlockGraph::_clone_ins(Ins &ins) {
  // clone ins (and all its children) for every user of ins
  // keep the same order when cloning an ins
  // (I think it should preserve program order)
  auto preds = ins.preds;

  for (std::size_t i = 1; i < preds.size(); ++i) {
    auto &user = *preds[i];
    auto &new_node = _clone_node(ins);
    user.replace_succ(ins, new_node);
  }
}
void BlockGraph::_extract_ins(Ins &ins) {
  auto def_reg = ins.def;
  assert(!def_reg.empty());
  auto preds = ins.preds;

  // Replace ins with a reg to for all ops
  for (auto p : preds) {
    auto &user = dynamic_cast<Ins &>(*p);
    auto &new_node = _add_reg(def_reg);
    user.replace_succ(ins, new_node);
  }

  // Nothing else to do
  // Ins has no preds, so it's now a new root tree
  assert(ins.preds.empty());
}

BlockGraph::Node &BlockGraph::_clone_node(Node &node) {
  if (auto b = dynamic_cast<BlockRef *>(&node))
    return _add_block(b->name);
  if (auto c = dynamic_cast<Const *>(&node))
    return _add_const(c->val);

  if (auto r = dynamic_cast<Reg *>(&node))
    return _add_reg(r->reg);

  auto &ins = dynamic_cast<Ins &>(node);
  std::vector<Node *> new_succs;
  auto new_def = ins.def;
  for (auto succ : node.succs) {
    if (succ)
      new_succs.push_back(&_clone_node(*succ));
    else
      new_succs.push_back(nullptr);
  }

  auto new_ins = std::make_unique<Ins>(new_succs, ins.opname, ins.args, new_def,
                                       ins.order);
  auto &res = *new_ins;
  _nodes.push_back(std::move(new_ins));

  // if (!new_def.empty())
  //  _regs_nodes[new_def] = &res;

  return res;
}

BlockGraph::BlockRef &BlockGraph::_add_block(const std::string &name) {
  auto node = std::make_unique<BlockRef>(name);
  auto ptr = node.get();
  _nodes.push_back(std::move(node));
  return *ptr;
}

BlockGraph::Const &BlockGraph::_add_const(long val) {
  auto node = std::make_unique<Const>(val);
  auto ptr = node.get();
  _nodes.push_back(std::move(node));
  return *ptr;
}

BlockGraph::Reg &BlockGraph::_add_reg(const std::string &name) {
  auto node = std::make_unique<Reg>(name);
  auto ptr = node.get();
  _nodes.push_back(std::move(node));
  return *ptr;
}

void BlockGraph::_add_extern_reg(const std::string &name) {
  assert(_regs_nodes.count(name) == 0);
  auto &r = _add_reg(name);
  _regs_nodes.emplace(name, &r);
}

std::string BlockGraph::_node_label(const Node &node) const {
  std::string res = "";
  if (auto ins = dynamic_cast<const Ins *>(&node)) {
    res += FMT_OSS("Ins(" << ins->opname << ")");
    if (!ins->def.empty())
      res += FMT_OSS(" (" << ins->def << ")");
  } else if (auto b = dynamic_cast<const BlockRef *>(&node))
    res += FMT_OSS("Block(" << b->name << ")");
  else if (auto c = dynamic_cast<const Const *>(&node))
    res += FMT_OSS("Const(" << c->val << ")");
  else if (auto c = dynamic_cast<const Const *>(&node))
    res += FMT_OSS("Const(" << c->val << ")");
  else if (auto r = dynamic_cast<const Reg *>(&node))
    res += FMT_OSS("Reg(" << r->reg << ")");
  else
    assert(0);

  return res;
}
