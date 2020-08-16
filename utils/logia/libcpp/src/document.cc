#include <logia/document.hh>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <random>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

#include <logia/doc-config.hh>

#define DATA_DIR (CMAKE_SRC_DIR "/../data")

namespace logia {

namespace {

long get_time_ms() {
  using namespace std::chrono;
  milliseconds ms =
      duration_cast<milliseconds>(system_clock::now().time_since_epoch());
  return ms.count();
}

bool is_legal_id_char(char c) {
  if (c >= 'a' && c <= 'z')
    return true;
  if (c >= 'A' && c <= 'Z')
    return true;
  if (c >= '0' && c <= '9')
    return true;
  return c == '_';
}

std::string gen_uid() {
  constexpr std::size_t UID_SIZE = 24;

  static std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<> dist(0, 128);

  std::string res;
  while (res.size() < UID_SIZE) {
    auto c = dist(gen);
    if (is_legal_id_char(c))
      res.push_back(c);
  }

  return res;
}

void create_dir(const std::string &path) {
  if (mkdir(path.c_str(), S_IRWXU) < 0) {
    std::cerr << "MDLogger: Failed to create document directory `" << path
              << "': " << strerror(errno)
              << " (Maybe document ID is already taken ?)\n";
    std::abort();
  }
}

std::string build_data_dir(const std::string &id) {
  auto name = std::string(DATA_DIR) + "/doc_" + id;
  create_dir(name);
  return name;
}

} // namespace

Document::Document(const std::string &name, const std::string &type,
                   bool link_in_root) {
  _conf.id = gen_uid();
  _conf.link_in_root = link_in_root;
  _conf.name = name;
  _conf.time = get_time_ms();
  _conf.type = type;

  _dir = build_data_dir(_conf.id);
  _md_path = _dir + "/main.md";
  _os.open(_md_path);
  _unique = 0;

  auto conf_path = _dir + "/config.txt";
  _conf.write_to_file(conf_path);
}

Document::~Document() {

  if (!_os.good()) {
    std::cerr << "MDLogger: Failed to write markdown file  `" << _md_path << "'"
              << std::endl;
    std::abort();
  }
}

std::string Document::gen_file_name(const std::string &post) {
  return std::to_string(_unique++) + post;
}

std::string Document::gen_file_path(const std::string &post) {
  return _dir + "/" + gen_file_name(post);
}

std::string Document::get_file_path(const std::string &filename) const {
  return "/docs/doc_" + id() + "/" + filename;
}

} // namespace logia
