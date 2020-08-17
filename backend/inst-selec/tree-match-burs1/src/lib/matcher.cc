#include "matcher.hh"

#include <cassert>

#include "../utils/digraph.hh"
#include <logia/program.hh>
#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>
#include <utils/str/str.hh>

namespace {

struct DotGraphMatchVisitor : public MatchVisitor {

  std::map<const BlockGraph::Node *, std::string> labels;

  std::string label_prefix(const BlockGraph::Node &node) {
    std::string res = "";
    if (auto ins = dynamic_cast<const BlockGraph::Ins *>(&node))
      res += FMT_OSS("Ins(" << ins->opname << ")");
    else if (auto b = dynamic_cast<const BlockGraph::BlockRef *>(&node))
      res += FMT_OSS("Block(" << b->name << ")");
    else if (auto c = dynamic_cast<const BlockGraph::Const *>(&node))
      res += FMT_OSS("Const(" << c->val << ")");
    else if (auto r = dynamic_cast<const BlockGraph::Reg *>(&node))
      res += FMT_OSS("Reg(" << r->reg << ")");
    else
      assert(0);

    res += ": ";
    return res;
  }

  void before(const BlockGraph::Node &node, MatchEntry e) override {
    const auto &r = *e.rule;
    auto lbl = labels[&node];
    if (lbl.empty())
      lbl = label_prefix(node);
    if (lbl.back() == '>')
      lbl += "+ ";

    lbl += FMT_OSS("<" << r.lhs() << "#" << r.idx() << ", " << e.cost << ">");
    labels[&node] = lbl;
  }
};

#if 0
class Rewriter : public MatchVisitor {

  struct Extra {
    std::string def_reg;
  };

public:
  Rewriter(BasicBlock &bb) : _bb(bb), _next_tmp_reg(0) {}

  void before(const BlockGraph::Node &node, MatchEntry e) override {
    const auto &r = *e.rule;
    if (r.lhs() == "root") {
      const auto &ins = dynamic_cast<const BlockGraph::Ins &>(node);
      if (!ins.def.empty()) {
        // std::cout << "good root for " << r std::endl;
        _extra[&node].def_reg = ins.def;
      }
    }
  }

  void after(const BlockGraph::Node &node, MatchEntry e) override {
    const auto &r = *e.rule;
    // Parse and add code

    for (auto args : r.code()) {
      for (auto &arg : args) {
        if (arg == "$$") {
          if (_extra.count(&node)) {
            // don't generate tmp reg
            arg = "%" + _extra.at(&node).def_reg;
          } else
            arg = "%" + _gen_tmp_reg();
        } else if (arg[0] == '$')
          arg = _rewrite_arg(node, arg.c_str() + 1);
      }

      _bb.add_ins(args);

      // Track def reg for usage in parents nodes
      isa::Ins cins(_bb.parent().parent().ctx(), args);
      auto def_idx = get_def_idx(cins);
      if (def_idx != DEF_IDX_NONE) {
        auto &extra = _extra[&node];
        extra.def_reg = cins.args()[def_idx + 1].substr(1);
      }
    }
  }

private:
  BasicBlock &_bb;
  std::size_t _next_tmp_reg;
  std::map<const BlockGraph::Node *, Extra> _extra;

  std::string _gen_tmp_reg() { return "t" + std::to_string(_next_tmp_reg++); }

  std::string _rewrite_arg(const BlockGraph::Node &node, const char *fmt) {
    // parser for $x.y.z

    if (*fmt) { // Find end of chain
      int num = *(fmt++) - '0';
      assert(num >= 0 && num < 9);

      const auto &child = *node.succs.at(num);

      if (*fmt)
        assert(*(fmt++) == '.');
      return _rewrite_arg(child, fmt);
    }

    // leaf

    // Special case for block
    if (auto b = dynamic_cast<const BlockGraph::BlockRef *>(&node))
      return '@' + b->name;

    // special case for constant and regs
    const auto &extra = _extra[&node];
    if (extra.def_reg.empty()) {
      if (auto c = dynamic_cast<const BlockGraph::Const *>(&node))
        return std::to_string(c->val);
      if (auto r = dynamic_cast<const BlockGraph::Reg *>(&node))
        return "%" + r->reg;
    }

    assert(!extra.def_reg.empty());
    return "%" + extra.def_reg;
  }
};
#endif

} // namespace

bool MatchInfos::add_match(const Rule &r, int cost) {
  auto it = _rc.find(r.lhs());
  if (it != _rc.end() && it->second.cost <= cost)
    return false;

  _rc[r.lhs()] = MatchEntry(&r, cost);
  return true;
}

MatchEntry MatchInfos::get_match(const std::string &rule_name) const {
  auto it = _rc.find(rule_name);
  return it == _rc.end() ? MatchEntry{} : it->second;
}

Matcher::Matcher(const Rules &rules, const BlockGraph &bg,
                 const BlockGraph::Ins &root)
    : _rules(rules), _bg(bg), _root(root) {

  for (auto node : _bg.get_nodes_in(_root))
    _infos.emplace(node, MatchInfos{*node});

  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(
      "Matching Tree: @" + _bg.bb().parent().name() + ":@" + _bg.bb().name());
}

void Matcher::run() {
  _match_node(_root);
  const auto &infos = _infos.at(&_root);

  if (infos.get_match("root").cost == MAX_COST) {
    *_doc << "## Full Match Tree\n";
    get_match_graph_all().dump_tree(*_doc);
    _doc->raw_os().flush();
    PANIC("Failed to match tree");
  }

  *_doc << "## Match Tree\n";
  get_match_graph().dump_tree(*_doc);
  _doc->raw_os().flush();
}

void Matcher::apply(MatchVisitor &v) const { _apply_rec(v, _root, "root"); }

#if 0
void Matcher::rewrite(BasicBlock &out_bb) const {
  std::size_t code_start = out_bb.code().size();

  Rewriter v(out_bb);
  apply(v);

  *_doc << "## Generated ASM\n";
  auto ch = _doc->code("asm");
  for (std::size_t i = code_start; i < out_bb.code().size(); ++i) {
    const auto &ins = out_bb.code()[i];
    ch << "    ";
    for (std::size_t i = 0; i < ins.size(); ++i) {
      if (i == 1)
        ch << " ";
      else if (i > 1)
        ch << ", ";
      ch << ins[i];
    }
    ch << "\n";
  }
}
#endif

Digraph Matcher::get_match_graph_all() const {
  Digraph g = _bg.to_graph(_root);
  VertexAdapter<const BlockGraph::Node *> va(_bg.get_nodes_in(_root));
  for (std::size_t u = 0; u < va.size(); ++u)
    g.labels_set_vertex_name(u, _node_label_all(*va(u)));
  return g;
}

Digraph Matcher::get_match_graph() const {
  DotGraphMatchVisitor v;
  apply(v);

  Digraph g = _bg.to_graph(_root);
  VertexAdapter<const BlockGraph::Node *> va(_bg.get_nodes_in(_root));
  for (std::size_t u = 0; u < va.size(); ++u)
    g.labels_set_vertex_name(u, v.labels.at(va(u)));
  return g;
}

void Matcher::_match_node(const BlockGraph::Node &node) {
  if (auto b = dynamic_cast<const BlockGraph::BlockRef *>(&node))
    _match_block(*b);
  else if (auto c = dynamic_cast<const BlockGraph::Const *>(&node))
    _match_const(*c);
  else if (auto r = dynamic_cast<const BlockGraph::Reg *>(&node))
    _match_reg(*r);
  else if (auto i = dynamic_cast<const BlockGraph::Ins *>(&node))
    _match_ins(*i);
}

void Matcher::_match_block(const BlockGraph::BlockRef &node) {
  // leaf basicblock ref, only match __block__ pattern
  auto leaf = _rules.get_leaf_block(node.name);

  for (const auto &r : _rules.rules())
    if (r->is_op() && r->get_op() == leaf)
      _add_match(node, *r, r->cost());
}

void Matcher::_match_const(const BlockGraph::Const &node) {
  // leaf const, only match __const__ pattern
  auto leaf = _rules.get_leaf_const(node.val);

  for (const auto &r : _rules.rules())
    if (r->is_op() && r->get_op() == leaf)
      _add_match(node, *r, r->cost());
}

void Matcher::_match_reg(const BlockGraph::Reg &node) {
  // leaf reg, only match __reg__ pattern
  auto leaf = _rules.get_leaf_reg(node.reg);

  for (const auto &r : _rules.rules())
    if (r->is_op() && r->get_op() == leaf)
      _add_match(node, *r, r->cost());
}

void Matcher::_match_ins(const BlockGraph::Ins &node) {
  // Match childrens first
  for (auto c : node.succs)
    _match_node(*c);

  for (const auto &r : _rules.rules()) {
    if (r->is_nt()) // non-terminal rules matched indirectly
      continue;
    if (r->get_op() != node.opname) // wrong operator
      continue;

    // Try to match rules for all children
    int cost = r->cost();
    bool valid = true;
    assert(node.succs.size() + 1 == r->get_op_args().size());
    for (std::size_t i = 0; i < node.succs.size(); ++i) {
      const auto &arg_infos = _infos.at(node.succs[i]);
      const auto &arg_rule = r->get_arg(i);
      int arg_cost = arg_infos.get_match(arg_rule).cost;

      if (arg_cost == MAX_COST) { // child not matched
        valid = false;
        break;
      }

      cost += arg_cost;
    }

    if (valid)
      _add_match(node, *r, cost);
  }
}

void Matcher::_add_match(const BlockGraph::Node &node, const Rule &rule,
                         int cost) {
  auto &infos = _infos.at(&node);
  if (!infos.add_match(rule, cost))
    return;

  // Propagate infos to all nt rules
  for (const auto &ntr : _rules.rules())
    if (ntr->is_nt() && ntr->get_nt() == rule.lhs())
      _add_match(node, *ntr, cost + ntr->cost());
}

void Matcher::_apply_rec(MatchVisitor &v, const BlockGraph::Node &node,
                         const std::string &rule) const {
  // Get match
  const auto &infos = _infos.at(&node);
  auto match = infos.get_match(rule);
  assert(match.rule);

  // List all nt rules
  std::vector<MatchEntry> nt_rules;
  while (match.rule->is_nt()) {
    nt_rules.push_back(match);
    match = infos.get_match(match.rule->get_nt());
    assert(match.rule);
  }

  // Apply n nt rules
  for (auto r : nt_rules)
    v.before(node, r);

  // Apply to all children
  const auto &r = *match.rule;
  assert(r.is_op());
  v.before(node, match);
  assert(node.succs.size() + 1 == r.get_op_args().size());
  for (std::size_t i = 0; i < node.succs.size(); ++i)
    _apply_rec(v, *node.succs[i], r.get_arg(i));
  v.after(node, match);

  // Apply n nt rules
  for (auto r : nt_rules)
    v.after(node, r);
}

std::string Matcher::_node_label_all(const BlockGraph::Node &node) const {
  std::string res = "";
  if (auto ins = dynamic_cast<const BlockGraph::Ins *>(&node))
    res += FMT_OSS("Ins(" << ins->opname << ")");
  else if (auto b = dynamic_cast<const BlockGraph::BlockRef *>(&node))
    res += FMT_OSS("Block(" << b->name << ")");
  else if (auto c = dynamic_cast<const BlockGraph::Const *>(&node))
    res += FMT_OSS("Const(" << c->val << ")");
  else if (auto r = dynamic_cast<const BlockGraph::Reg *>(&node))
    res += FMT_OSS("Reg(" << r->reg << ")");
  else
    assert(0);

  res += ": {";

  const auto &infos = _infos.at(&node);
  for (const auto &rc : infos.all_matches())
    res += FMT_OSS("<" << rc.second.rule->lhs() << "#" << rc.second.rule->idx()
                       << ", " << rc.second.cost << "> ");

  res += "}";
  return res;
}

#if 0

bool Matcher::_has_match(const BlockGraph::Node &node) const {
  const auto &infos = _matches[_va(&node)];
  return infos.best_cost != MAX_COST;
}

Matcher::MatchInfos Matcher::_best_match(const BlockGraph::Node &node) const {
  assert(_has_match(node));
  const auto &infos = _matches[_va(&node)];
  for (const auto &m : infos.matches) {
    if (m.total_cost == infos.best_cost)
      return m;
  }

  assert(0);
}

Matcher::MatchInfos Matcher::_best_match_for(const BlockGraph::Node &node,
                                             const std::string &rule) const {
  MatchInfos best;
  best.rule = -1;
  best.total_cost = MAX_COST;
  const auto &infos = _matches[_va(&node)];
  for (const auto &m : infos.matches) {
    const auto &r = _rules.rules()[m.rule];
    if (r.name == rule && m.total_cost < best.total_cost)
      best = m;
  }
  return best;
}

void Matcher::_match_graph_rec(Digraph &g, const BlockGraph::Node &node,
                               const std::string &rule) const {
  auto match = _best_match_for(node, rule);
  assert(match.total_cost != MAX_COST);
  const auto &r = _rules.rules()[match.rule];

  // Match current node
  g.labels_set_vertex_name(_va(&node), _node_label(node, rule));

  // Match all children
  assert(node.succs.size() + 1 == r.pat.size());
  for (std::size_t i = 0; i < node.succs.size(); ++i)
    _match_graph_rec(g, *node.succs[i], r.pat[i + 1]);
}

std::string Matcher::_node_label(const BlockGraph::Node &node,
                                 const std::string &rule) const {
  std::string res = "";
  if (auto ins = dynamic_cast<const BlockGraph::Ins *>(&node))
    res += FMT_OSS("Ins(" << ins->opname << ")");
  else if (auto b = dynamic_cast<const BlockGraph::BlockRef *>(&node))
    res += FMT_OSS("Block(" << b->name << ")");
  else if (auto c = dynamic_cast<const BlockGraph::Const *>(&node))
    res += FMT_OSS("Const(" << c->val << ")");
  else if (auto r = dynamic_cast<const BlockGraph::Reg *>(&node))
    res += FMT_OSS("Reg(" << r->reg << ")");
  else
    assert(0);

  if (!_has_match(node))
    res += ": <X>";
  else {
    auto match = rule.empty() ? _best_match(node) : _best_match_for(node, rule);
    res += FMT_OSS(": <" << _rules.rules()[match.rule].name << "#" << match.rule
                         << ", " << match.total_cost << ">");
  }

  const auto &infos = _matches[_va(&node)];
  if (!infos.def_reg.empty())
    res += FMT_OSS(" (%" << infos.def_reg << ")");

  return res;
}

#endif
