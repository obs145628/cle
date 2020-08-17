#include "rules.hh"

#include <fstream>
#include <iostream>

#include <logia/md-gfm-doc.hh>
#include <logia/program.hh>
#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>
#include <utils/str/str.hh>

namespace {

constexpr const char *DEFAULT_RULES = ""
                                      "@b ; __block__ ; 0 ; |\n"
                                      "@c ; __const__ ; 0 ; |\n"
                                      "@r ; __reg__ ; 0 ; |\n";
}

std::ostream &operator<<(std::ostream &os, const Rule &r) {
  os << "  " << r.idx << ": " << r.name << " ; ";

  for (const auto &s : r.pat)
    os << s << " ";
  for (const auto &p : r.props)
    os << ":" << p.first << "=" << p.second << " ";
  os << "; ";

  os << r.cost << " ; ";

  for (const auto &ins : r.code) {
    for (const auto &s : ins)
      os << s << " ";
    os << "| ";
  }
  return os;
}

Rules::Rules(const std::string &path) {
  std::ifstream is(path);
  PANIC_IF(!is.good(), FMT_OSS("Invalid rules file `" << path << "'"));

  // Parse rules file
  std::string l;
  while (std::getline(is, l)) {
    l = utils::str::trim(l);
    if (!l.empty())
      _parse_rule(l);
  }

  // Parse default rules
  for (auto l : utils::str::split(DEFAULT_RULES, '\n')) {
    l = utils::str::trim(l);
    if (!l.empty())
      _parse_rule(l);
  }

  _extend();

  for (std::size_t i = 0; i < _rules.size(); ++i)
    _rules[i].idx = i;

  auto doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(
      "Tree-Matching Rules");
  auto ch = doc->code();
  ch.os() << *this;

  check();
}

void Rules::check() const {
  for (const auto &r : _rules) {
    if (r.name[0] == '@')
      PANIC(FMT_OSS("Rule " << r.name << ": is an alias"));

    for (std::size_t i = 1; i < r.pat.size(); ++i) {
      const auto &s = r.pat[i];
      if (s[0] == '@')
        PANIC(FMT_OSS("Rule " << r.name << ": use alias " << s));
      if (s[0] == '*')
        continue;

      bool found = false;
      for (const auto &r : _rules)
        if (r.name == s) {
          found = true;
          break;
        }
      if (!found)
        PANIC(FMT_OSS("Rule " << r.name << ": use undefined rule " << s));
    }
  }
}

void Rules::_parse_rule(const std::string &l) {
  auto args = utils::str::split(l, ';');
  PANIC_IF(args.size() != 4,
           FMT_OSS("Invalid nummber of arguments in rule `" << l << "'"));

  Rule rule;
  rule.name = utils::str::trim(args[0]);

  for (auto arg : utils::str::split(utils::str::trim(args[1]), ' ')) {
    if (arg[0] == ':') { // property
      auto prop = utils::str::split(arg.substr(1), '=');
      PANIC_IF(prop.size() != 2,
               FMT_OSS("Invalid property " << arg << " in rule `" << l << "'"));
      rule.props.emplace(prop[0], prop[1]);
    } else {
      rule.pat.push_back(arg);
    }
  }

  rule.cost = utils::str::parse_long(utils::str::trim(args[2]));

  for (auto ins : utils::str::split(utils::str::trim(args[3]), '|')) {
    auto args = utils::str::split(utils::str::trim(ins), ' ');
    if (!args.empty())
      rule.code.push_back(args);
  }

  _rules.push_back(rule);
}

void Rules::_extend() {

  // Extend to get the real set of rules
  for (std::size_t i = 0; i < _rules.size(); ++i) {
    const auto &r = _rules[i];
    if (r.pat.size() == 1 && r.pat[0][0] == '@') {
      _extend_alias(r);
      // Remove alias
      _rules.erase(_rules.begin() + i);
      --i;
    }
  }

  // Remove all aliases rules
  std::vector<Rule> new_rules;
  for (const auto &r : _rules)
    if (r.name[0] != '@')
      new_rules.push_back(r);
  _rules = new_rules;
}

void Rules::_extend_alias(const Rule &ar) {
  const auto &alias = ar.pat[0];

  std::vector<Rule> new_rules;

  for (const auto &r : _rules) {
    if (r.name != alias)
      continue;

    Rule new_rule;
    new_rule.name = ar.name;
    new_rule.pat = r.pat;
    new_rule.props = ar.props;
    new_rule.cost = r.cost + ar.cost;
    new_rule.code = r.code;
    new_rule.code.insert(new_rule.code.end(), ar.code.begin(), ar.code.end());
    new_rules.push_back(new_rule);
  }

  _rules.insert(_rules.end(), new_rules.begin(), new_rules.end());
}

std::ostream &operator<<(std::ostream &os, const Rules &rules) {
  for (const auto &r : rules.rules())
    os << r << "\n";
  return os;
}
