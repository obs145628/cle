#pragma once

#include "rules.hh"
#include "tree-block.hh"
#include <gop10/module.hh>

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

  Matcher(const Rules &rules, const TreeBlock &tb);

  void run();
  void rewrite(gop::Module &out_mod, const isa::Context &ctx);

  void dump_tree(const std::string &path);

private:
  const Rules &_rules;
  const TreeBlock &_tb;
  std::size_t _next_tmp_reg;

  std::vector<NodeInfos> _matches;

  void _match_node(const TreeBlock::Node &node);
  void _match_const(const TreeBlock::Const &node);
  void _match_reg(const TreeBlock::Reg &node);
  void _match_ins(const TreeBlock::Ins &node);

  void _add_match(std::size_t node_idx, std::size_t rule_idx, int total_cost);

  bool _has_match(const TreeBlock::Node &node) const;
  MatchInfos _best_match(const TreeBlock::Node &node) const;
  MatchInfos _best_match_for(const TreeBlock::Node &node,
                             const std::string &rule) const;

  void _rewrite(gop::Module &out_mod, const isa::Context &ctx,
                const TreeBlock::Node &node, const std::string &rule);

  std::string _node_label(const TreeBlock::Node &node) const;

  const std::string &_gen_tmp_reg(std::string &buff);
  std::string _gen_tmp_reg();

  std::string _rewrite_arg(const TreeBlock::Node &node, const char *fmt) const;
};
