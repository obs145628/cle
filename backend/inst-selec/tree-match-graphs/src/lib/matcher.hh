#pragma once

#include "../utils/digraph.hh"
#include "../utils/vertex-adapter.hh"
#include "block-graph.hh"
#include "module.hh"
#include "rules.hh"
#include <logia/md-gfm-doc.hh>

constexpr int MAX_COST = 1 << 30;

class Matcher {

public:
  struct MatchInfos {
    std::size_t rule;
    int total_cost;
  };

  struct NodeInfos {
    std::vector<MatchInfos> matches;
    int best_cost;
    std::string def_reg;

    NodeInfos() : best_cost(MAX_COST) {}
  };

  Matcher(const Rules &rules, const BlockGraph &bg,
          const BlockGraph::Ins &root);

  void run();
  void rewrite(BasicBlock &out_bb);

  // Only shows best match considering lower levels, not uper ones
  Digraph get_local_match_graph() const;
  Digraph get_match_graph() const;

private:
  const Rules &_rules;
  const BlockGraph &_bg;
  const BlockGraph::Ins &_root;
  VertexAdapter<const BlockGraph::Node *> _va;
  std::size_t _next_tmp_reg;

  std::vector<NodeInfos> _matches;

  std::unique_ptr<logia::MdGfmDoc> _doc;

  void _match_node(const BlockGraph::Node &node);
  void _match_block(const BlockGraph::BlockRef &node);
  void _match_const(const BlockGraph::Const &node);
  void _match_reg(const BlockGraph::Reg &node);
  void _match_ins(const BlockGraph::Ins &node);

  void _add_match(const BlockGraph::Node &node, std::size_t rule_idx,
                  int total_cost);

  bool _has_match(const BlockGraph::Node &node) const;
  MatchInfos _best_match(const BlockGraph::Node &node) const;
  MatchInfos _best_match_for(const BlockGraph::Node &node,
                             const std::string &rule) const;

  void _rewrite(BasicBlock &out_bb, const BlockGraph::Node &node,
                const std::string &rule);

  std::string _node_label(const BlockGraph::Node &node,
                          const std::string &rule = "") const;

  const std::string &_gen_tmp_reg(std::string &buff);
  std::string _gen_tmp_reg();

  std::string _rewrite_arg(const BlockGraph::Node &node, const char *fmt) const;

  void _match_graph_rec(Digraph &g, const BlockGraph::Node &node,
                        const std::string &rule) const;
};
