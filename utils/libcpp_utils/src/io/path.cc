#include <utils/io/path.hh>

#include <cstdlib>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>

namespace utils {
namespace path {

std::string join(const std::string &p1, const std::string &p2) {
  if (p2.size() > 0 && p2[0] == '/')
    return p2;

  if (p1.size() > 0 && p1.back() == '/')
    return p1 + p2;
  else
    return p1 + "/" + p2;
}

std::pair<std::string, std::string> split_ext(const std::string &path) {
  auto spos = path.rfind('.');
  if (spos == std::string::npos)
    return std::make_pair(path, std::string{});
  else
    return std::make_pair(path.substr(0, spos), path.substr(spos));
}

std::string abspath(const std::string &path) {
  char path_buf[PATH_MAX];
  PANIC_IF(realpath(path.c_str(), path_buf) == NULL,
           FORMAT_STRING("abspath " << path << " failed."));
  return path_buf;
}

bool is_file(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) < 0)
    return false;

  return S_ISREG(st.st_mode);
}

bool is_dir(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) < 0)
    return false;

  return S_ISDIR(st.st_mode);
}

} // namespace path
} // namespace utils
