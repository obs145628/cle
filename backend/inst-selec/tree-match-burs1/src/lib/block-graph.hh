#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../isa/isa.hh"
#include "../utils/digraph.hh"
#include "live-out.hh"
#include "module.hh"

// Rebuild linear basicblock as a digraph
class BlockGraph {
public:
  struct Node {
    Node(const std::vector<Node *> &succs);
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;
    virtual ~Node() {}

    std::vector<Node *> preds; // (users)
    std::vector<Node *> succs; // operands (usees)

    std::size_t depth() const;

    void replace_succ(Node &old_succ, Node &new_succ);
  };

  // Constant in the IR code
  struct Const : public Node {
    virtual ~Const() {}
    Const(long val) : Node({}), val(val) {}
    long val;
  };

  // Opaque register. can be one of :
  // - function argument
  // - reg computed in another block
  // - reg computed in this block, but extracted to turn digraph into forest of
  // trees
  struct Reg : public Node {
    virtual ~Reg() {}
    Reg(const std::string &reg) : Node({}), reg(reg) {}

    std::string reg;
  };

  // Reference to a basicblock for branch instructions
  struct BlockRef : public Node {
    virtual ~BlockRef() {}
    BlockRef(const std::string &name) : Node({}), name(name) {}

    std::string name;
  };

  // IR instruction
  // If there is a def argument, it's removed from the list
  // If it has no preds, it's a tree root that will be translated
  struct Ins : public Node {
    virtual ~Ins() {}
    Ins(const std::vector<Node *> &succs, const isa::Ins ins, int order);
    Ins(const std::vector<Node *> &succs, const std::string &opname,
        const std::vector<isa::arg_kind_t> &args, const std::string &def,
        int order);

    std::string opname;
    std::vector<isa::arg_kind_t> args;
    std::string def; // reg name, or empty if no def
    int order;       // position in IR code, ensure exececution order
  };

  BlockGraph(const BasicBlock &bb);

  std::size_t count() const { return _nodes.size(); }
  std::vector<const Node *> nodes_list() const;

  const BasicBlock &bb() const { return _bb; }

  // Transform digraph into a forest of trees
  void to_trees();

  Digraph to_graph() const;
  Digraph to_graph(const Ins &root) const;

  // Return list of roots trees, sorted by exec order
  std::vector<const Ins *> get_roots() const;

  // Return all nodes belonging to tree starting at root
  std::vector<const Node *> get_nodes_in(const Ins &root) const;

private:
  const BasicBlock &_bb;
  const isa::Context &_ctx;
  const LiveOut &_live_out;

  std::vector<std::unique_ptr<Node>> _nodes;
  std::map<std::string, Node *> _regs_nodes;

  // Build graph
  void _build();
  Node *_build_arg(const isa::Ins &ins, std::size_t idx);
  Ins *_build_ins(const isa::Ins &ins, std::size_t order);
  std::vector<std::string> _get_extern_block_regs();

  // transform to forest of trees
  void _extract_node(Node &node);
  void _clone_ins(Ins &ins);
  void _extract_ins(Ins &ins);
  Node &_clone_node(Node &node);

  BlockRef &_add_block(const std::string &name);
  Const &_add_const(long val);
  Reg &_add_reg(const std::string &name);
  void _add_extern_reg(const std::string &name);

  std::string _node_label(const Node &node) const;
};
