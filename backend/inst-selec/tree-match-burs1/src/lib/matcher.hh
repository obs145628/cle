#pragma once

#include <map>

#include "../utils/digraph.hh"
#include "../utils/vertex-adapter.hh"
#include "block-graph.hh"
#include "module.hh"
#include "rules.hh"
#include <logia/md-gfm-doc.hh>

constexpr int MAX_COST = 1 << 30;

struct MatchEntry {
  const Rule *rule;
  int cost;

  MatchEntry() : MatchEntry(nullptr, MAX_COST) {}
  MatchEntry(const Rule *rule, int cost) : rule(rule), cost(cost) {}
};

struct MatchInfos {
public:
  MatchInfos(const BlockGraph::Node &node) : _node(node) {}

  const BlockGraph::Node &node() const { return _node; }

  bool add_match(const Rule &r, int cost);

  MatchEntry get_match(const std::string &rule_name) const;

  bool is_matched() const { return !_rc.empty(); }

  const std::map<std::string, MatchEntry> &all_matches() const { return _rc; }

private:
  const BlockGraph::Node &_node;
  std::map<std::string, MatchEntry> _rc;
};

class MatchVisitor {

public:
  virtual ~MatchVisitor() {}

  // Called first, when going down the match tree
  virtual void before(const BlockGraph::Node &, MatchEntry) {}

  // Called second, when going back up the match tree
  virtual void after(const BlockGraph::Node &, MatchEntry) {}
};

class Matcher {

public:
  Matcher(const Rules &rules, const BlockGraph &bg,
          const BlockGraph::Ins &root);

  void run();
  void apply(MatchVisitor &v) const;

  Digraph get_match_graph_all() const;

  Digraph get_match_graph() const;

  logia::MdGfmDoc &get_md_doc() const { return *_doc; }

private:
  const Rules &_rules;
  const BlockGraph &_bg;
  const BlockGraph::Ins &_root;
  std::map<const BlockGraph::Node *, MatchInfos> _infos;
  std::size_t _next_tmp_reg;

  std::unique_ptr<logia::MdGfmDoc> _doc;

  void _match_node(const BlockGraph::Node &node);
  void _match_block(const BlockGraph::BlockRef &node);
  void _match_const(const BlockGraph::Const &node);
  void _match_reg(const BlockGraph::Reg &node);
  void _match_ins(const BlockGraph::Ins &node);

  void _add_match(const BlockGraph::Node &node, const Rule &rule, int cost);

  void _apply_rec(MatchVisitor &v, const BlockGraph::Node &node,
                  const std::string &rule) const;

  std::string _node_label_all(const BlockGraph::Node &node) const;
};
