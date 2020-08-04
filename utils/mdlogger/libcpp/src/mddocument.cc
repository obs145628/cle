#include <mdlogger/mddocument.hh>

#include <cstdlib>
#include <iostream>

MDDocument::MDDocument(const std::string &out_dir)
    : _dir(out_dir), _os(_dir + "/main.md"), _unique(0) {}

MDDocument::~MDDocument() {

  if (!_os.good()) {
    std::cerr << "MDLogger: Failed to write data file in `" << _dir << "'"
              << std::endl;
    std::abort();
  }
}

void MDDocument::code_begin(const std::string &language) {
  raw_os() << "```" << language << "\n";
}

void MDDocument::code_end() { raw_os() << "```\n"; }

std::string MDDocument::gen_file_name(const std::string &post) {
  return std::to_string(_unique++) + post;
}

std::string MDDocument::gen_file_path(const std::string &post) {
  return _dir + "/" + gen_file_name(post);
}

MDDocument::OsWrapperHelper<MDDocument::CodeHelper>
MDDocument::code(const std::string &language) {
  code_begin(language);
  return OsWrapperHelper<CodeHelper>(*this);
}

void MDDocument::image(const std::string &label, const std::string path) {
  raw_os() << "![" << label << "](" << path << ")\n";
}
