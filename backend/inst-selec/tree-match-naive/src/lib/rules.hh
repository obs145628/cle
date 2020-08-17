#pragma once

#include <map>
#include <string>
#include <vector>

struct Rule {
  using ins_t = std::vector<std::string>;
  using pattern_t = std::vector<std::string>;

  std::size_t idx;
  std::string name;
  pattern_t pat;
  std::map<std::string, std::string> props;
  int cost;
  std::vector<ins_t> code;
};

class Rules {
public:
  Rules(const std::string &path);

  const std::vector<Rule> &rules() const { return _rules; }

  void check() const;

  void dump(std::ostream &os) const;

private:
  std::vector<Rule> _rules;

  void _parse_rule(const std::string &l);
  void _extend();
  void _extend_alias(const Rule &r);
};
