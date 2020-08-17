#pragma once

#include "matcher.hh"
#include "module.hh"

struct RewriterErrCtx {
  const BlockGraph::Node *node;
  const Rule *rule;
  const RuleCode::Operation *op;
  const std::string *op_arg;

  RewriterErrCtx()
      : node(nullptr), rule(nullptr), op(nullptr), op_arg(nullptr) {}

  void err(const std::string &msg);
};

class CodeArg {

public:
  virtual ~CodeArg() {}

  CodeArg(const RewriterErrCtx &err_ctx) : _err_ctx(err_ctx) {}

  virtual std::string read_val() = 0;
  virtual void update_val(const std::string &new_val) = 0;

private:
  RewriterErrCtx _err_ctx;

protected:
  void _err(const std::string &msg) { _err_ctx.err(msg); }
};

class Rewriter : public MatchVisitor {

public:
  struct NodeInfos {
    std::string def_reg;
  };

  Rewriter(BasicBlock &bb);

  void run(const Matcher &m);

  void before(const BlockGraph::Node &node, MatchEntry e) override;

  void after(const BlockGraph::Node &node, MatchEntry e) override;

private:
  BasicBlock &_bb;
  std::map<const BlockGraph::Node *, NodeInfos> _infos;
  std::size_t _next_tmp;

  RewriterErrCtx _err_ctx;

  std::unique_ptr<CodeArg> _parse_arg(const char *fmt,
                                      const BlockGraph::Node &node, bool root);
  std::unique_ptr<CodeArg> _parse_arg(const std::string &str,
                                      const BlockGraph::Node &node);

  void _exec_set(const BlockGraph::Node &node, const RuleCode::Operation &op);
  void _exec_emit(const BlockGraph::Node &node, const RuleCode::Operation &op);
};
