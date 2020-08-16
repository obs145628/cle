#include <logia/program.hh>

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>

#include <logia/md-gfm-doc.hh>

namespace logia {

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

Program &Program::instance() {
  static Program res;
  return res;
}

void Program::set_command(int argc, char **argv) {
  g_argc = argc;
  g_argv = argv;
}

Program::Program() {
  assert(g_argc && g_argv);

  auto main_name = get_cmd_line();
  _main_doc = std::make_unique<MdGfmDoc>(main_name, true);
}

Program::~Program() {}

void Program::_on_add(const Document &doc) {
  *_main_doc << "- ";
  _main_doc->doc_link(doc);
  *_main_doc << "\n";
  _main_doc->raw_os().flush();
}

} // namespace logia
