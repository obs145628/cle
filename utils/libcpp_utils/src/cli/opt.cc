#include <utils/cli/opt.hh>

namespace utils {

Opt::Opt(int argc, char **argv) : _args(argv, argv + argc) {}

bool Opt::has_flag(char sflag, const std::string &lflag) const {
  std::string smatch;
  if (sflag != '\0') {
    smatch.push_back('-');
    smatch.push_back(sflag);
  }
  std::string lmatch;
  if (!lflag.empty())
    lmatch = "--" + lflag;

  for (const auto &arg : _args)
    if (!arg.empty() && (arg == smatch || arg == lmatch))
      return true;

  return false;
}

bool Opt::has_flag(const std::string &lflag) const {
  return has_flag('\0', lflag);
}

} // namespace utils
