#include "rewriter.hh"

#include <cassert>

#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>
#include <utils/str/str.hh>

namespace {

class CodeArgConst : public CodeArg {

public:
  CodeArgConst(const RewriterErrCtx &err_ctx, const std::string &str)
      : CodeArg(err_ctx), _str(str) {}

  std::string read_val() override { return _str; }

  void update_val(const std::string &) override {
    _err("cannot update read-only value");
  }

private:
  std::string _str;
};

bool is_field_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9');
}

std::string parse_field(const char *&fmt) {
  std::string res;
  while (is_field_char(*fmt))
    res.push_back(*(fmt++));
  return res;
}

class CodeArgDef : public CodeArg {

public:
  CodeArgDef(const RewriterErrCtx &err_ctx, Rewriter::NodeInfos &infos,
             bool is_mut, std::size_t *next_tmp)
      : CodeArg(err_ctx), _infos(infos), _is_mut(is_mut), _next_tmp(next_tmp) {}

  std::string read_val() override {
    if (_infos.def_reg.empty()) {
      if (!_next_tmp)
        _err("Trying to read node with empty def_reg");
      auto res = "%t" + std::to_string((*_next_tmp)++);
      update_val(res);
      return res;
    }
    return "%" + _infos.def_reg;
  }

  void update_val(const std::string &new_val) override {
    if (!_is_mut)
      _err("Trying to update def_reg of unmutable node");
    if (!_infos.def_reg.empty())
      _err("Trying to update def_reg of node, but already set");

    _infos.def_reg = new_val.front() == '%' ? new_val.substr(1) : new_val;
  }

private:
  Rewriter::NodeInfos &_infos;
  bool _is_mut;
  std::size_t *_next_tmp;
};

} // namespace

void RewriterErrCtx::err(const std::string &msg) {
  std::cerr << "Rewriter failed: " << msg << "\n";

  if (node)
    std::cerr << "  Node: " << node << "\n";
  if (rule)
    std::cerr << "  Rule: `" << *rule << "'\n";
  if (op)
    std::cerr << "  Op: `" << *op << "'\n";
  if (op_arg)
    std::cerr << "  Op argument: `" << *op_arg << "'\n";

  PANIC("Compilation aborted !");
}

Rewriter::Rewriter(BasicBlock &bb) : _bb(bb), _next_tmp(0) {}

void Rewriter::run(const Matcher &m) {
  std::size_t code_start = _bb.code().size();
  auto &doc = m.get_md_doc();
  m.apply(*this);

  doc << "## Generated ASM\n";
  auto ch = doc.code("asm");
  for (std::size_t i = code_start; i < _bb.code().size(); ++i) {
    const auto &ins = _bb.code()[i];
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

void Rewriter::before(const BlockGraph::Node &node, MatchEntry e) {
  _err_ctx.rule = e.rule;
  const auto &code = e.rule->code();

  for (const auto &op : code.ops) {
    if (op.name == "set") {
      _exec_set(node, op);
    } else if (op.name == "emit") {

    } else {
      PANIC(
          FMT_OSS("Unknown operation `" << op.name << "' in rule " << *e.rule));
    }
  }
}

void Rewriter::after(const BlockGraph::Node &node, MatchEntry e) {
  _err_ctx.rule = e.rule;
  const auto &code = e.rule->code();
  for (const auto &op : code.ops) {
    if (op.name == "set") {

    } else if (op.name == "emit") {
      _exec_emit(node, op);
    } else {
      PANIC(
          FMT_OSS("Unknown operation `" << op.name << "' in rule " << *e.rule));
    }
  }
}

std::unique_ptr<CodeArg>
Rewriter::_parse_arg(const char *fmt, const BlockGraph::Node &node, bool root) {
  if (!*fmt)
    _err_ctx.err("op argument value cannot be a node");

  if (!root && *(fmt++) != '.')
    _err_ctx.err("Expected '.' in op argument");

  auto field = parse_field(fmt);
  if (field.empty())
    _err_ctx.err("Invalid empty field in op argument");

  // Recurse on node child
  if (isdigit(field[0])) {
    std::size_t child_idx = utils::str::parse_long(field);
    if (child_idx >= node.succs.size())
      _err_ctx.err(FMT_OSS("Op trying to access node child #"
                           << child_idx << ", only has " << node.succs.size()));
    return _parse_arg(fmt, *node.succs[child_idx], false);
  }

  if (field == "val") {
    if (auto c = dynamic_cast<const BlockGraph::Const *>(&node))
      return std::make_unique<CodeArgConst>(_err_ctx, std::to_string(c->val));
    _err_ctx.err("Usage of field `val' for not const node in op argument");
  }

  if (field == "def") {
    if (auto i = dynamic_cast<const BlockGraph::Ins *>(&node)) {
      if (i->def.empty())
        _err_ctx.err(
            "Usage of field `def' for ins without def reg in op argument");
      return std::make_unique<CodeArgConst>(_err_ctx, "%" + i->def);
    }
    _err_ctx.err("Usage of field `def' for not ins node in op argument");
  }

  if (field == "name") {
    if (auto b = dynamic_cast<const BlockGraph::BlockRef *>(&node))
      return std::make_unique<CodeArgConst>(_err_ctx, "@" + b->name);
    if (auto r = dynamic_cast<const BlockGraph::Reg *>(&node))
      return std::make_unique<CodeArgConst>(_err_ctx, "%" + r->reg);
    _err_ctx.err("Usage of field `name' for not block/reg node in argument");
  }

  if (field == "D") {
    auto &infos = _infos[&node];
    return std::make_unique<CodeArgDef>(
        _err_ctx, infos, /*is_mut=*/true,
        /*tmp_reg=*/root ? &_next_tmp : nullptr);
  }

  _err_ctx.err("Usage of unknown field `" + field + "' in op argument");
  return nullptr;
}

std::unique_ptr<CodeArg> Rewriter::_parse_arg(const std::string &str,
                                              const BlockGraph::Node &node) {

  _err_ctx.node = &node;
  _err_ctx.op_arg = &str;

  if (str.front() == '$')
    return _parse_arg(str.data() + 1, node, true);
  else
    return std::make_unique<CodeArgConst>(_err_ctx, str);
}

void Rewriter::_exec_set(const BlockGraph::Node &node,
                         const RuleCode::Operation &op) {
  _err_ctx.op = &op;
  PANIC_IF(op.args.size() != 2, "set operation must have 2 arguments");
  auto dst = _parse_arg(op.args[0], node);
  auto val = _parse_arg(op.args[1], node);
  dst->update_val(val->read_val());
}

void Rewriter::_exec_emit(const BlockGraph::Node &node,
                          const RuleCode::Operation &op) {
  _err_ctx.op = &op;
  std::vector<std::string> args;
  for (const auto &arg : op.args)
    args.push_back(_parse_arg(arg, node)->read_val());
  _bb.add_ins(args);
}
