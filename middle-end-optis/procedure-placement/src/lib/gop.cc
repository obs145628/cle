#include "gop.hh"

#include <utils/cli/err.hh>
#include <utils/str/str.hh>

#include <cassert>

namespace gop {

std::unique_ptr<Ins> Ins::parse(const std::string &str) {
  auto nend = str.find(' ');
  if (nend == std::string::npos)
    return std::make_unique<Ins>(std::vector<std::string>{str});

  auto opname = str.substr(0, nend);
  auto args = utils::str::split(utils::str::trim(str.substr(nend + 1)), ',');
  for (auto &arg : args)
    arg = utils::str::trim(arg);
  args.insert(args.begin(), opname);

  return std::make_unique<Ins>(args);
}

void Ins::dump(std::ostream &os) const {
  os << args[0] << ' ';

  for (std::size_t i = 1; i < args.size(); ++i) {
    os << args[i];
    if (i + 1 < args.size())
      os << ", ";
  }
}

std::unique_ptr<Dir> Dir::parse(const std::string &str) {
  assert(str.size() > 1 && str[0] == '.');
  auto nend = str.find(' ');
  if (nend == std::string::npos)
    return std::make_unique<Dir>(str.substr(1), std::vector<std::string>{});

  auto name = str.substr(1, nend - 1);
  auto args = utils::str::split(utils::str::trim(str.substr(nend + 1)), ',');
  for (auto &arg : args)
    arg = utils::str::trim(arg);

  return std::make_unique<Dir>(name, args);
}

void Dir::dump(std::ostream &os) const {
  os << '.' << name << ' ';

  for (std::size_t i = 0; i < args.size(); ++i) {
    os << args[i];
    if (i + 1 < args.size())
      os << ", ";
  }
}

Module Module::parse(std::istream &is) {
  Module mod;

  std::vector<std::string> label_defs;
  std::vector<std::string> comm_pre;

  std::string line;
  while (std::getline(is, line)) {
    line = utils::str::trim(line);
    if (line.empty())
      continue;

    if (line.front() == ';') { // full line comment
      auto comm = line.substr(1);
      comm_pre.push_back(comm);
      continue;
    }

    if (line.back() == ':') { // label def
      auto label = line.substr(0, line.size() - 1);
      assert(!label.empty());
      label_defs.push_back(label);
      continue;
    }

    std::unique_ptr<Decl> decl;

    // parse optional EOL comment
    std::string comm_eol;
    auto comd = line.find(';');
    if (comd != std::string::npos) {
      comm_eol = line.substr(comd + 1);
      line = line.substr(0, comd);
    }

    if (line[0] == '.') // directive
      decl = Dir::parse(line);
    else // instruction
      decl = Ins::parse(line);

    decl->label_defs = label_defs;
    label_defs.clear();
    decl->comm_pre = comm_pre;
    comm_pre.clear();
    decl->comm_eol = comm_eol;
    mod.decls.push_back(std::move(decl));
  }

  assert(label_defs.empty());
  return mod;
}

void Module::dump(std::ostream &os) const {
  for (const auto &dec : decls) {

    if (!dec->label_defs.empty()) {
      os << "\n";
      for (const auto &label : dec->label_defs)
        os << label << ":\n";
    }

    for (const auto &comm : dec->comm_pre)
      os << " ; " << comm << "\n";

    if (dynamic_cast<const Ins *>(dec.get()))
      os << "\t";
    dec->dump(os);

    if (!dec->comm_eol.empty())
      os << " ; " << dec->comm_eol;
    os << "\n";
  }
}

} // namespace gop
