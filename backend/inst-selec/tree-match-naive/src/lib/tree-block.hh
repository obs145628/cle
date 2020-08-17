#pragma once

#include <map>
#include <memory>

#include "../isa/isa.hh"
#include <gop10/module.hh>

constexpr std::size_t ID_NONE = -1;

class TreeBlock {

public:
  struct Node {
    Node() {}
    virtual ~Node() {}

    std::size_t idx;
    std::size_t parent;
    std::vector<std::size_t> children;
  };

  struct Const : public Node {
    virtual ~Const() {}
    long val;
  };

  struct Reg : public Node {
    virtual ~Reg() {}
    std::string reg;
  };

  struct Ins : public Node {
    virtual ~Ins() {}

    Ins(isa::Ins ins) : ins(ins) {}

    isa::Ins ins;
  };

  TreeBlock(const isa::Context &ctx, const gop::Module &mod);

  std::size_t count() const { return _nodes.size(); }
  const Node &get(std::size_t idx) const { return *_nodes.at(idx); }
  const Node &root() const { return get(_root); }

  void dump_tree(const std::string &out_path) const;

private:
  std::vector<std::unique_ptr<Node>> _nodes;
  std::size_t _root;
};
