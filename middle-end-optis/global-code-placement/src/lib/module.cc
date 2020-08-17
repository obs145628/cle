#include "module.hh"

#include <utils/str/str.hh>

#include <cassert>

Ins Ins::parse(const std::string &str_all) {
  // Parse optional eol comment
  std::string str = str_all;
  std::string comm;
  auto comd = str.find(';');
  if (comd != std::string::npos) {
    comm = str.substr(comd + 1);
    str = str.substr(0, comd);
  }

  auto nend = str.find(' ');
  if (nend == std::string::npos) {
    Ins res;
    res.args = {str};
    return res;
  }

  auto opname = str.substr(0, nend);
  auto args = utils::str::split(utils::str::trim(str.substr(nend + 1)), ',');
  for (auto &arg : args)
    arg = utils::str::trim(arg);

  Ins res;
  res.args = {opname};
  res.comm_eol = comm;
  res.args.insert(res.args.end(), args.begin(), args.end());
  return res;
}

void Ins::dump(std::ostream &os) const {
  os << args[0];
  if (!args.empty())
    os << ' ';

  for (std::size_t i = 1; i < args.size(); ++i) {
    os << args[i];
    if (i + 1 < args.size())
      os << ", ";
  }
}

Module Module::parse(std::istream &is) {
  Module mod;

  std::vector<std::string> label_defs;

  std::string line;
  while (std::getline(is, line)) {
    line = utils::str::trim(line);
    if (line.empty())
      continue;

    if (line.back() == ':') {
      auto label = line.substr(0, line.size() - 1);
      auto pos = mod.code.size();
      label_defs.push_back(label);
      mod.labels.emplace(label, pos);
    }

    else {
      auto ins = Ins::parse(line);
      ins.label_defs = label_defs;
      label_defs.clear();
      mod.code.push_back(ins);
    }
  }

  assert(label_defs.empty());
  return mod;
}

void Module::dump(std::ostream &os) const {

  for (std::size_t i = 0; i < code.size(); ++i) {
    if (i > 0 && code[i].label_defs.size() > 0)
      os << "\n";
    for (const auto &label : code[i].label_defs)
      os << label << ":\n";

    os << "\t";
    code[i].dump(os);
    if (!code[i].comm_eol.empty())
      os << " ;" << code[i].comm_eol;
    os << "\n";
  }
}
