#include "matcher.hh"

#include <cassert>
#include <fstream>

#include "../utils/digraph.hh"
#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>
#include <utils/str/str.hh>

namespace {

gop::Ins *add_ins(gop::Module &mod, const std::vector<std::string> &args) {
  auto ins = std::make_unique<gop::Ins>(args);
  mod.decls.push_back(std::move(ins));
  return dynamic_cast<gop::Ins *>(mod.decls.back().get());
}

std::size_t get_def_idx(const isa::Ins &ins) {
  for (std::size_t i = 0; i < ins.args().size() - 1; ++i)
    if (ins.get_arg_kind(i) == isa::ARG_KIND_DEF ||
        ins.get_arg_kind(i) == isa::ARG_KIND_USEDEF)
      return i;
  return ID_NONE;
}

} // namespace

Matcher::Matcher(const Rules &rules, const TreeBlock &tb)
    : _rules(rules), _tb(tb), _matches(_tb.count()) {}

void Matcher::run() {
  _match_node(_tb.root());
  const auto &infos = _matches[_tb.root().idx];
  dump_tree("ir_match.dot");
  PANIC_IF(infos.best_cost == MAX_COST, "Failed to match tree");
}

void Matcher::rewrite(gop::Module &out_mod, const isa::Context &ctx) {
  std::size_t start = out_mod.decls.size();
  _next_tmp_reg = 0;
  _rewrite(out_mod, ctx, _tb.root(), "root");
  out_mod.decls.at(start)->label_defs.push_back("B0");
  dump_tree("ir_match.dot");
}

void Matcher::dump_tree(const std::string &out_path) {
  Digraph g(_tb.count());
  for (std::size_t u = 0; u < _tb.count(); ++u) {
    const auto &node = _tb.get(u);
    g.labels_set_vertex_name(node.idx, _node_label(node));
    if (node.parent != ID_NONE)
      g.add_edge(node.parent, node.idx);
  }

  std::ofstream os(out_path);
  g.dump_tree(os);
}

void Matcher::_match_node(const TreeBlock::Node &node) {

  if (auto c = dynamic_cast<const TreeBlock::Const *>(&node))
    _match_const(*c);
  else if (auto r = dynamic_cast<const TreeBlock::Reg *>(&node))
    _match_reg(*r);
  else if (auto i = dynamic_cast<const TreeBlock::Ins *>(&node))
    _match_ins(*i);
}

void Matcher::_match_const(const TreeBlock::Const &node) {
  // leaf const, only match __const__ (@c) pattern

  for (const auto &r : _rules.rules()) {
    if (r.pat.size() != 1 || r.pat[0] != "__const__")
      continue;

    auto it = r.props.find("val");
    if (it != r.props.end() && utils::str::parse_long(it->second) != node.val)
      continue;

    _add_match(node.idx, r.idx, r.cost);
  }
}

void Matcher::_match_reg(const TreeBlock::Reg &node) {
  // leaf reg, only match __reg__ (@r) pattern

  auto &infos = _matches[node.idx];
  infos.def_reg = node.reg;

  for (const auto &r : _rules.rules()) {
    if (r.pat.size() != 1 || r.pat[0] != "__reg__")
      continue;

    _add_match(node.idx, r.idx, r.cost);
  }
}

void Matcher::_match_ins(const TreeBlock::Ins &node) {
  bool is_root = node.parent == ID_NONE;

  // Match childrens first
  for (auto c : node.children)
    if (c != ID_NONE)
      _match_node(_tb.get(c));

  for (const auto &r : _rules.rules()) {
    if (is_root && r.name != "root")
      continue;
    if (r.pat[0] != node.ins.opname())
      continue;

    // Try to match rules for all children
    int cost = r.cost;
    bool valid = true;
    for (std::size_t i = 0; i < node.ins.args().size() - 1; ++i) {
      const auto &pat = r.pat[i + 1];
      if (pat == "*") // match everything
        continue;

      if (node.children[i] == ID_NONE) { // cannot match this one
        valid = false;
        break;
      }

      int child_cost =
          _best_match_for(_tb.get(node.children[i]), pat).total_cost;
      if (child_cost == MAX_COST) { // child not matched
        valid = false;
        break;
      }

      cost += child_cost;
    }

    if (valid)
      _add_match(node.idx, r.idx, cost);
  }
}

void Matcher::_add_match(std::size_t node_idx, std::size_t rule_idx,
                         int total_cost) {
  NodeInfos &infos = _matches[node_idx];
  MatchInfos match;
  match.rule = rule_idx;
  match.total_cost = total_cost;
  infos.matches.push_back(match);
  infos.best_cost = std::min(infos.best_cost, match.total_cost);
}

bool Matcher::_has_match(const TreeBlock::Node &node) const {
  const auto &infos = _matches[node.idx];
  return infos.best_cost != MAX_COST;
}

Matcher::MatchInfos Matcher::_best_match(const TreeBlock::Node &node) const {
  assert(_has_match(node));
  const auto &infos = _matches[node.idx];
  for (const auto &m : infos.matches) {
    if (m.total_cost == infos.best_cost)
      return m;
  }

  assert(0);
}

Matcher::MatchInfos Matcher::_best_match_for(const TreeBlock::Node &node,
                                             const std::string &rule) const {
  MatchInfos best;
  best.rule = -1;
  best.total_cost = MAX_COST;
  const auto &infos = _matches[node.idx];
  for (const auto &m : infos.matches) {
    const auto &r = _rules.rules()[m.rule];
    if (r.name == rule && m.total_cost < best.total_cost)
      best = m;
  }
  return best;
}

void Matcher::_rewrite(gop::Module &out_mod, const isa::Context &ctx,
                       const TreeBlock::Node &node, const std::string &rule) {
  auto match = _best_match_for(node, rule);
  assert(match.total_cost != MAX_COST);
  const auto &r = _rules.rules()[match.rule];

  // Rewrite all children
  for (std::size_t i = 0; i < r.pat.size() - 1; ++i) {
    const auto &pat = r.pat[i + 1];
    if (pat == "*") // no rewriting to do
      continue;

    assert(node.children[i] != ID_NONE);
    _rewrite(out_mod, ctx, _tb.get(node.children[i]), pat);
  }

  // Parse and add code
  for (auto args : r.code) {
    for (auto &arg : args) {
      if (arg == "$$")
        arg = "%" + _gen_tmp_reg();
      else if (arg[0] == '$')
        arg = _rewrite_arg(node, arg.c_str() + 1);
    }

    auto ins = add_ins(out_mod, args);

    // Track def reg for usage in parents nodes
    isa::Ins cins(ctx, ins->args);
    auto def_idx = get_def_idx(cins);
    if (def_idx != ID_NONE) {
      auto &infos = _matches[node.idx];
      infos.def_reg = cins.args()[def_idx + 1].substr(1);
    }
  }
}

std::string Matcher::_node_label(const TreeBlock::Node &node) const {
  std::string res = FMT_OSS(node.idx << ": ");
  if (auto ins = dynamic_cast<const TreeBlock::Ins *>(&node))
    res += FMT_OSS("Ins(" << ins->ins.opname() << ")");
  else if (auto c = dynamic_cast<const TreeBlock::Const *>(&node))
    res += FMT_OSS("Const(" << c->val << ")");
  else if (auto r = dynamic_cast<const TreeBlock::Reg *>(&node))
    res += FMT_OSS("Reg(" << r->reg << ")");
  else
    assert(0);

  if (!_has_match(node))
    res += ": <X>";
  else {
    auto match = _best_match(node);
    res += FMT_OSS(": <" << _rules.rules()[match.rule].name << "#" << match.rule
                         << ", " << match.total_cost << ">");
  }

  const auto &infos = _matches[node.idx];
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

std::string Matcher::_rewrite_arg(const TreeBlock::Node &node,
                                  const char *fmt) const {
  // parser for $x.y.z

  if (*fmt) { // Find end of chain
    int num = *(fmt++) - '0';
    assert(num >= 0 && num < 9);

    const auto &child = _tb.get(node.children.at(num));

    if (*fmt)
      assert(*(fmt++) == '.');
    return _rewrite_arg(child, fmt);
  }

  // leaf

  // special case for constant
  const auto &infos = _matches[node.idx];
  if (infos.def_reg.empty())
    if (auto c = dynamic_cast<const TreeBlock::Const *>(&node))
      return std::to_string(c->val);

  assert(!infos.def_reg.empty());
  return "%" + infos.def_reg;
}
