#include <utils/io/file.hh>

#include <cstdio>

#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>

namespace utils {

void write_file_str(const std::string &path, const std::string &content) {
  FILE *os = std::fopen(path.c_str(), "wb");
  PANIC_IF(!os, FORMAT_STRING("Couldn't open file " << path));
  PANIC_IF(std::fwrite(content.c_str(), 1, content.size(), os) !=
               content.size(),
           FORMAT_STRING("Couldn't write to " << path));
  std::fclose(os);
}

std::string read_file_str(const std::string &path) {
  FILE *is = std::fopen(path.c_str(), "rb");
  PANIC_IF(!is, FORMAT_STRING("Couldn't open file " << path));
  PANIC_IF(std::fseek(is, 0, SEEK_END) != 0, "fseek failed");
  std::size_t len = std::ftell(is);
  PANIC_IF(std::fseek(is, 0, SEEK_SET) != 0, "fseek failed");

  std::string res;
  res.resize(len);
  PANIC_IF(std::fread(&res[0], 1, len, is) != len,
           FORMAT_STRING("Couldn't read " << path));
  std::fclose(is);
  return res;
}

} // namespace utils
