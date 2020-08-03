#include <utils/io/tmp.hh>

#include <ctime>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

#include <utils/io/file.hh>

namespace utils {

namespace {
constexpr std::size_t ID_NO = static_cast<std::size_t>(-1);
}

/// Create a unique temporary path in /tmp folder
std::string make_tmp_path(const std::string &prefix,
                          const std::string &postfix) {
  static std::size_t dir_id = ID_NO;
  static std::size_t file_id = 0;
  static std::string tmp_dir;

  if (dir_id == ID_NO) {
    // @EXTRA: do better random
    dir_id = std::clock() % 10000;
    std::ostringstream os;
    os << "/tmp/gbx_cl_" << std::hex << dir_id << "/";
    tmp_dir = os.str();
    mkdir(tmp_dir.c_str(), 0766);
  }

  return tmp_dir + prefix + "_" + std::to_string(++file_id) + "_" + postfix;
}

std::string make_tmp_text_file(const std::string &content,
                               const std::string &prefix,
                               const std::string &postfix) {
  auto path = make_tmp_path(prefix, postfix);
  write_file_str(path, content);
  return path;
}

} // namespace utils
