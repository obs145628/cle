#include "rules.hh"

#include <fstream>
#include <iostream>

#include <logia/md-gfm-doc.hh>
#include <logia/program.hh>
#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>
#include <utils/str/str.hh>

namespace {

constexpr std::size_t RULE_NONE = -1;

constexpr char CHAR_EOF = '#';

constexpr const char *LEAF_NAME_BLOCK = "__block__";
constexpr const char *LEAF_NAME_CONST_ANY = "__const__x";
constexpr const char *LEAF_NAME_CONST_PRE = "__const__";
constexpr const char *LEAF_NAME_REG = "__reg__";

bool is_id_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

RuleCode::Operation parse_rule_operation(const std::string &str) {
  auto name_end = str.find('(');
  PANIC_IF(name_end == std::string::npos || str.back() != ')',
           "Invalid rule operation syntax: `" + str + "'");

  RuleCode::Operation res;
  res.name = str.substr(0, name_end);
  res.args = utils::str::split(
      str.substr(name_end + 1, str.size() - name_end - 2), ',');
  return res;
}

RuleCode parse_rule_code(std::string str) {
  RuleCode res;
  str = utils::str::replace(str, ' ', "");
  for (const auto &op : utils::str::split(str, '|'))
    if (!op.empty())
      res.ops.push_back(parse_rule_operation(op));
  return res;
}

} // namespace

class RulesCharStream {
public:
  RulesCharStream(const std::string &str) : _str(str), _pos(0) {}

  char peek() const { return _pos == _str.size() ? CHAR_EOF : _str[_pos]; }
  char get() { return _pos == _str.size() ? CHAR_EOF : _str[_pos++]; }

  std::string get_word() {
    std::string res;
    while (is_id_char(peek()))
      res.push_back(get());
    return res;
  }

private:
  const std::string &_str;
  std::size_t _pos;
};

std::ostream &operator<<(std::ostream &os, const RuleCode::Operation &op) {
  os << op.name << "(";
  for (std::size_t i = 0; i < op.args.size(); ++i) {
    os << op.args[i];
    if (i + 1 < op.args.size())
      os << ", ";
  }
  return os << ")";
}

std::ostream &operator<<(std::ostream &os, const RuleCode &code) {
  for (std::size_t i = 0; i < code.ops.size(); ++i) {
    os << code.ops[i];
    if (i + 1 < code.ops.size())
      os << "; ";
  }

  return os;
}

Rule::Rule(const std::string &lhs, const std::string &rhs, int cost,
           const RuleCode &code)
    : _idx(RULE_NONE), _lhs(lhs), _rhs({rhs}), _rhs_op(false), _cost(cost),
      _code(code) {}

Rule::Rule(const std::string &lhs, const std::vector<std::string> &rhs,
           int cost, const RuleCode &code)
    : _idx(RULE_NONE), _lhs(lhs), _rhs(rhs), _rhs_op(true), _cost(cost),
      _code(code) {}

std::ostream &operator<<(std::ostream &os, const Rule &r) {
  os << "#" << r.idx() << ": " << r.lhs() << " -> ";

  if (r.is_nt())
    os << r.get_nt();
  else {
    const auto &args = r.get_op_args();
    os << args[0] << "(";
    for (std::size_t i = 1; i < args.size(); ++i) {
      os << args[i];
      if (i + 1 < args.size())
        os << ", ";
    }
    os << ")";
  }

  return os << " (" << r.cost() << ") {" << r.code() << "}";
}

Rules::Rules(const isa::Context &input_ctx, const std::string &path)
    : _in_ctx(input_ctx), _next_gen(0) {
  std::ifstream is(path);
  _parse_is(is);

  auto doc =
      logia::Program::instance().add_doc<logia::MdGfmDoc>("Tree-Maching rules");
  auto ch = doc->code();
  ch.os() << *this;

  check();
}

void Rules::check() const {
  for (const auto &r : _rules) {
    if (r->is_nt()) {
      PANIC_IF(!rule_exists(r->get_nt()),
               FMT_OSS("Use of undefined non-terminal `"
                       << r->get_nt() << "' in nt-rule `" << *r << "'"));
    }

    else {
      const auto &args = r->get_op_args();
      PANIC_IF(!_is_operator(args[0]),
               FMT_OSS("Use of undefined operator `"
                       << args[0] << "' in op-rule `" << *r << "'"));
      for (std::size_t i = 1; i < args.size(); ++i)
        PANIC_IF(!rule_exists(args[i]),
                 FMT_OSS("Use of undefined non-terminal `"
                         << args[i] << "' in nt-rule `" << *r << "'"));
    }
  }
}

bool Rules::rule_exists(const std::string &name) const {
  for (const auto &r : _rules)
    if (r->lhs() == name)
      return true;
  return false;
}

std::string Rules::get_leaf_const(long val) const {
  if (!_explicit_consts.count(val))
    return LEAF_NAME_CONST_ANY;
  else
    return LEAF_NAME_CONST_PRE + std::to_string(val);
}

std::string Rules::get_leaf_reg(const std::string &) const {
  return LEAF_NAME_REG;
}

std::string Rules::get_leaf_block(const std::string &) const {
  return LEAF_NAME_BLOCK;
}

void Rules::_parse_rhs(const std::string &lhs, int cost, const RuleCode &code,
                       RulesCharStream &rhs, std::string first) {
  if (first.empty())
    first = rhs.get_word();
  assert(!first.empty());

  if (!_is_operator(first)) {
    // Parsed non-terminal rule
    _rules.push_back(std::make_unique<Rule>(lhs, first, cost, code));
    return;
  }

  if (rhs.peek() != '(') {
    // Parsed leaf op
    _rules.push_back(std::make_unique<Rule>(
        lhs, std::vector<std::string>{first}, cost, code));
    return;
  }

  assert(rhs.get() == '(');
  std::vector<std::string> args{first};

  while (rhs.peek() != ')') {
    auto arg = rhs.get_word();
    assert(!arg.empty());

    if (_is_operator(arg)) {
      // recurvise rule
      auto tmp_name = _gen_rule_name(lhs);
      _parse_rhs(tmp_name, 0, {}, rhs, arg);
      args.push_back(tmp_name);
    } else
      args.push_back(arg);

    if (rhs.peek() == ',')
      rhs.get();
  }
  assert(rhs.get() == ')');

  _rules.push_back(std::make_unique<Rule>(lhs, args, cost, code));
}

void Rules::_parse_rule(std::string l) {
  l = utils::str::trim(l);
  if (l.empty())
    return;

  auto args = utils::str::split(l, ';');
  PANIC_IF(args.size() != 4,
           FMT_OSS("Invalid nummber of arguments in rule `" << l << "'"));

  auto lhs = utils::str::trim(args[0]);
  auto rhs_str = utils::str::replace(args[1], ' ', "");
  RulesCharStream rhs(rhs_str);
  auto cost = utils::str::parse_long(utils::str::trim(args[2]));
  auto code = parse_rule_code(args[3]);
  _parse_rhs(lhs, cost, code, rhs, "");
}

void Rules::_parse_is(std::istream &is) {
  std::string l;
  while (std::getline(is, l))
    _parse_rule(l);

  for (std::size_t i = 0; i < _rules.size(); ++i)
    _rules[i]->_idx = i;
}

bool Rules::_is_operator(const std::string &symbol) const {
  if (symbol == LEAF_NAME_BLOCK || symbol == LEAF_NAME_CONST_ANY ||
      symbol == LEAF_NAME_REG)
    return true;

  if (utils::str::begins_with(symbol, LEAF_NAME_CONST_PRE)) {
    auto val_str = symbol.substr(std::string(LEAF_NAME_CONST_PRE).size());
    _explicit_consts.insert(utils::str::parse_long(val_str));
    return true;
  }

  if (_in_ctx.is_instruction(symbol))
    return true;

  return false;
}

std::string Rules::_gen_rule_name(const std::string &pre) {
  return pre + "_" + std::to_string(_next_gen++);
}

std::ostream &operator<<(std::ostream &os, const Rules &rules) {
  for (const auto &r : rules.rules())
    os << "  " << *r << "\n";
  return os;
}
