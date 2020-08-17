#pragma once

#include <cassert>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "../isa/isa.hh"

class Rules;
class RulesCharStream;

struct RuleCode {
  struct Operation {
    std::string name;
    std::vector<std::string> args;
  };

  std::vector<Operation> ops;
};

std::ostream &operator<<(std::ostream &os, const RuleCode::Operation &op);
std::ostream &operator<<(std::ostream &os, const RuleCode &code);

class Rule {
public:
  Rule(const std::string &lhs, const std::string &rhs, int cost,
       const RuleCode &code);
  Rule(const std::string &lhs, const std::vector<std::string> &rhs, int cost,
       const RuleCode &code);

  std::size_t idx() const { return _idx; }
  const std::string &lhs() const { return _lhs; }
  int cost() const { return _cost; }
  const RuleCode &code() const { return _code; }

  bool is_nt() const { return !_rhs_op; }
  bool is_op() const { return _rhs_op; }

  const std::string &get_nt() const {
    assert(is_nt());
    return _rhs[0];
  }

  const std::vector<std::string> &get_op_args() const {
    assert(is_op());
    return _rhs;
  }

  const std::string &get_op() const {
    assert(is_op());
    return _rhs[0];
  }

  const std::string &get_arg(std::size_t pos) const {
    assert(is_op());
    return _rhs.at(pos + 1);
  }

private:
  std::size_t _idx;
  std::string _lhs;
  std::vector<std::string> _rhs;
  bool _rhs_op;
  int _cost;
  RuleCode _code;

  friend class Rules;
};

std::ostream &operator<<(std::ostream &os, const Rule &rule);

class Rules {
public:
  Rules(const isa::Context &input_ctx, const std::string &path);

  std::size_t count() const { return _rules.size(); }
  const Rule &get(std::size_t idx) const { return *_rules.at(idx); }
  const std::vector<std::unique_ptr<Rule>> &rules() const { return _rules; }

  void check() const;

  bool rule_exists(const std::string &name) const;

  std::string get_leaf_const(long val) const;
  std::string get_leaf_reg(const std::string &name) const;
  std::string get_leaf_block(const std::string &name) const;

private:
  const isa::Context &_in_ctx;
  std::vector<std::unique_ptr<Rule>> _rules;
  mutable std::set<long> _explicit_consts;
  std::size_t _next_gen;

  void _parse_rhs(const std::string &lhs, int cost, const RuleCode &code,
                  RulesCharStream &rhs, std::string first);

  void _parse_rule(std::string l);

  void _parse_is(std::istream &is);

  bool _is_operator(const std::string &symbol) const;

  std::string _gen_rule_name(const std::string &pre);
};

std::ostream &operator<<(std::ostream &os, const Rules &rules);
