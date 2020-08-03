#include <utils/str/str.hh>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include <utils/cli/err.hh>

namespace {
bool is_wspace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
} // namespace

namespace utils {

namespace str {

std::string trim(const std::string &str) {

  std::size_t bpos = 0;
  while (bpos < str.length() && is_wspace(str[bpos]))
    ++bpos;
  if (bpos == str.length())
    return "";

  std::size_t epos = str.length() - 1;
  while (is_wspace(str[epos]))
    --epos;

  return std::string(str.begin() + bpos, str.begin() + epos + 1);
}

std::vector<std::string> split(const std::string &str, char sep) {
  std::vector<std::string> res;
  std::istringstream is(str);
  std::string val;
  while (std::getline(is, val, sep))
    res.push_back(val);
  return res;
}

long parse_long(const std::string &str) {
  char *end;
  auto res = std::strtol(str.c_str(), &end, 10);
  assert(*end == '\0');
  return res;
}

bool begins_with(const std::string &str, const std::string &pre) {
  return str.size() >= pre.size() &&
         std::equal(pre.begin(), pre.end(), str.begin());
}

bool ends_with(const std::string &str, const std::string &post) {
  return str.size() >= post.size() &&
         std::equal(post.begin(), post.end(),
                    str.begin() + (str.size() - post.size()));
}

std::string replace(const std::string &str, char pat, const std::string &rep) {
  std::string res;
  for (auto c : str) {
    if (c == pat)
      res += rep;
    else
      res.push_back(c);
  }
  return res;
}

} // namespace str

} // namespace utils
