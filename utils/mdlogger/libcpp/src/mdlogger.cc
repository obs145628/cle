#include <mdlogger/mdlogger.hh>

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>

namespace {

int g_argc = 0;
char **g_argv = nullptr;

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

MDLogger::MDLogger() {
  assert(g_argc && g_argv);

  auto main_name = get_cmd_line();
  _main_doc = std::make_unique<MDDocument>(main_name, "markdown", true);
}

std::unique_ptr<MDDocument> MDLogger::add_doc(const std::string &name) {
  auto res = std::make_unique<MDDocument>(name, "markdown", false);

  *_main_doc << "- ";
  _main_doc->doc_link(*res);
  *_main_doc << "\n";
  _main_doc->raw_os().flush();

  return res;
}
