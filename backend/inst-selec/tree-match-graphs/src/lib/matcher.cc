#include "matcher.hh"

#include <cassert>

#include "../utils/digraph.hh"
#include <logia/program.hh>
#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>
#include <utils/str/str.hh>

namespace {

constexpr std::size_t DEF_IDX_NONE = -1;

std::size_t get_def_idx(const isa::Ins &ins) {
  for (std::size_t i = 0; i < ins.args().size() - 1; ++i)
    if (ins.get_arg_kind(i) == isa::ARG_KIND_DEF ||
        ins.get_arg_kind(i) == isa::ARG_KIND_USEDEF)
      return i;
  return DEF_IDX_NONE;
}

} // namespace

Matcher::Matcher(const Rules &rules, const BlockGraph &bg,
                 const BlockGraph::Ins &root)
    : _rules(rules), _bg(bg), _root(root), _va(_bg.get_nodes_in(_root)),
      _matches(_va.size()) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(
      "Matching Tree: @" + _bg.bb().parent().name() + ":@" + _bg.bb().name());
}

void Matcher::run() {
  _match_node(_root);
  const auto &infos = _matches[_va(&_root)];

  if (infos.best_cost == MAX_COST) {
    *_doc << "## Local Match Tree\n";
    get_local_match_graph().dump_tree(*_doc);
    _doc->raw_os().flush();
    PANIC("Failed to match tree");
  }

  *_doc << "## Match Tree\n";
  get_match_graph().dump_tree(*_doc);
}

void Matcher::rewrite(BasicBlock &out_bb) {
  _next_tmp_reg = 0;

  std::size_t code_start = out_bb.code().size();
  _rewrite(out_bb, _root, "root");

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

Digraph Matcher::get_local_match_graph() const {
  Digraph g = _bg.to_graph(_root);
  for (std::size_t u = 0; u < _va.size(); ++u)
    g.labels_set_vertex_name(u, _node_label(*_va(u)));
  return g;
}

Digraph Matcher::get_match_graph() const {
  Digraph g = _bg.to_graph(_root);
  _match_graph_rec(g, _root, "root");
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
  // leaf basicblock ref, only match __block__ (@b) pattern

  for (const auto &r : _rules.rules()) {
    if (r.pat.size() != 1 || r.pat[0] != "__block__")
      continue;

    _add_match(node, r.idx, r.cost);
  }
}

void Matcher::_match_const(const BlockGraph::Const &node) {
  // leaf const, only match __const__ (@c) pattern

  for (const auto &r : _rules.rules()) {
    if (r.pat.size() != 1 || r.pat[0] != "__const__")
      continue;

    auto it = r.props.find("val");
    if (it != r.props.end() && utils::str::parse_long(it->second) != node.val)
      continue;

    _add_match(node, r.idx, r.cost);
  }
}

void Matcher::_match_reg(const BlockGraph::Reg &node) {
  // leaf reg, only match __reg__ (@r) pattern

  auto &infos = _matches[_va(&node)];
  infos.def_reg = node.reg;

  for (const auto &r : _rules.rules()) {
    if (r.pat.size() != 1 || r.pat[0] != "__reg__")
      continue;

    _add_match(node, r.idx, r.cost);
  }
}

void Matcher::_match_ins(const BlockGraph::Ins &node) {
  bool is_root = node.preds.size() == 0;

  // Match childrens first
  for (auto c : node.succs)
    if (c)
      _match_node(*c);

  for (const auto &r : _rules.rules()) {
    if (is_root && r.name != "root")
      continue;
    if (r.pat[0] != node.opname)
      continue;

    // Try to match rules for all children
    int cost = r.cost;
    bool valid = true;
    for (std::size_t i = 0; i < node.args.size(); ++i) {
      const auto &pat = r.pat[i + 1];
      if (pat == "*") // match everything
        continue;

      if (!node.succs[i]) { // cannot match this one
        valid = false;
        break;
      }

      int child_cost = _best_match_for(*node.succs[i], pat).total_cost;
      if (child_cost == MAX_COST) { // child not matched
        valid = false;
        break;
      }

      cost += child_cost;
    }

    if (valid)
      _add_match(node, r.idx, cost);
  }
}

void Matcher::_add_match(const BlockGraph::Node &node, std::size_t rule_idx,
                         int total_cost) {
  NodeInfos &infos = _matches[_va(&node)];
  MatchInfos match;
  match.rule = rule_idx;
  match.total_cost = total_cost;
  infos.matches.push_back(match);
  infos.best_cost = std::min(infos.best_cost, match.total_cost);
}

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

void Matcher::_rewrite(BasicBlock &out_bb, const BlockGraph::Node &node,
                       const std::string &rule) {
  const auto &ctx = out_bb.parent().parent().ctx();
  auto match = _best_match_for(node, rule);
  assert(match.total_cost != MAX_COST);
  const auto &r = _rules.rules()[match.rule];

  // Rewrite all children
  for (std::size_t i = 0; i < r.pat.size() - 1; ++i) {
    const auto &pat = r.pat[i + 1];
    if (pat == "*") // no rewriting to do
      continue;

    assert(node.succs[i]);
    _rewrite(out_bb, *node.succs[i], pat);
  }

  // Parse and add code
  for (auto args : r.code) {
    for (auto &arg : args) {
      if (arg == "$$") {
        if (rule == "root") {
          // don't generate tmp reg
          const auto &ins = dynamic_cast<const BlockGraph::Ins &>(node);
          assert(!ins.def.empty());
          arg = "%" + ins.def;
        } else
          arg = "%" + _gen_tmp_reg();
      } else if (arg[0] == '$')
        arg = _rewrite_arg(node, arg.c_str() + 1);
    }

    out_bb.add_ins(args);

    // Track def reg for usage in parents nodes
    isa::Ins cins(ctx, args);
    auto def_idx = get_def_idx(cins);
    if (def_idx != DEF_IDX_NONE) {
      auto &infos = _matches[_va(&node)];
      infos.def_reg = cins.args()[def_idx + 1].substr(1);
    }
  }
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

const std::string &Matcher::_gen_tmp_reg(std::string &buff) {
  if (buff.empty())
    buff = _gen_tmp_reg();
  return buff;
}

std::string Matcher::_gen_tmp_reg() {
  return "t" + std::to_string(_next_tmp_reg++);
}

std::string Matcher::_rewrite_arg(const BlockGraph::Node &node,
                                  const char *fmt) const {
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

  // special case for constant
  const auto &infos = _matches[_va(&node)];
  if (infos.def_reg.empty())
    if (auto c = dynamic_cast<const BlockGraph::Const *>(&node))
      return std::to_string(c->val);

  assert(!infos.def_reg.empty());
  return "%" + infos.def_reg;
}

void Matcher::_match_graph_rec(Digraph &g, const BlockGraph::Node &node,
                               const std::string &rule) const {
  auto match = _best_match_for(node, rule);
  assert(match.total_cost != MAX_COST);
  const auto &r = _rules.rules()[match.rule];

  // Match current node
  g.labels_set_vertex_name(_va(&node), _node_label(node, rule));

  // Match all children
  for (std::size_t i = 0; i < r.pat.size() - 1; ++i) {
    const auto &pat = r.pat[i + 1];
    if (pat == "*") // mo children to match
      continue;

    assert(node.succs[i]);
    _match_graph_rec(g, *node.succs[i], pat);
  }
}
