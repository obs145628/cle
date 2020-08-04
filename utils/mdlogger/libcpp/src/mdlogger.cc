#include <mdlogger/mdlogger.hh>

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <sys/types.h>

#define DATA_DIR CMAKE_SRC_DIR "/../data/"

namespace {

long get_time_ms() {
  using namespace std::chrono;
  milliseconds ms =
      duration_cast<milliseconds>(system_clock::now().time_since_epoch());
  return ms.count();
}

std::string gen_dir_path(long timestamp) {
  return std::string(DATA_DIR) + "/entry_" + std::to_string(timestamp);
}

void create_dir(const std::string &path) { mkdir(path.c_str(), S_IRWXU); }

void write_conf(const std::string &path,
                const std::map<std::string, std::string> &data) {
  std::ofstream os(path);
  for (const auto &x : data)
    os << x.first << " = " << x.second << "\n";

  if (!os.good()) {
    std::cerr << "MDLogger: Failed to write config file `" << path << "'"
              << std::endl;
    std::abort();
  }
}

int g_argc;
char **g_argv;

std::string get_cmd_line() {
  std::string res = g_argv[0];
  for (int i = 1; i < g_argc; ++i)
    res += std::string(" ") + g_argv[i];
  return res;
}

} // namespace

MDLogger &MDLogger::instance() {
  static MDLogger res;
  return res;
}

void MDLogger::init(int argc, char **argv) {
  g_argc = argc;
  g_argv = argv;
}

MDLogger::MDLogger()
    : _timestamp(get_time_ms()), _dir(gen_dir_path(_timestamp)) {
  create_dir(_dir);

  auto conf_file = _dir + "/config.txt";
  std::map<std::string, std::string> conf;
  conf["time"] = std::to_string(_timestamp);
  conf["cmd"] = get_cmd_line();
  write_conf(conf_file, conf);
}

std::unique_ptr<MDDocument> MDLogger::add_doc(const std::string &name) {
  auto idx = _docs_count++;
  auto dir = _dir + "/doc_" + std::to_string(idx);
  auto conf_file = dir + "/config.txt";
  create_dir(dir);

  std::map<std::string, std::string> conf;
  conf["name"] = name;
  conf["order"] = std::to_string(idx);
  write_conf(conf_file, conf);

  return std::make_unique<MDDocument>(dir);
}
