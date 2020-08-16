#pragma once

#include <memory>
#include <string>

#include "fwd.hh"

namespace logia {

// Group of documents
// Represent the current program execution
// All documents created through program are not visible from root
// Instead it creates a special document visible in root, that contains a list
// of all those created documents.
class Program {
public:
  Program(const Program &) = delete;
  Program &operator=(Program &) = delete;

  ~Program();

  static Program &instance();

  // Must be called at the beginning of the program, before the first call to
  // instance
  static void set_command(int argc, char **argv);

  // Add a new document to the program output
  // T must a child class of Document
  template <class T> std::unique_ptr<T> add_doc(const std::string &name);

private:
  std::unique_ptr<MdGfmDoc> _main_doc;

  void _on_add(const Document &doc);

  Program();
};

template <class T>
inline std::unique_ptr<T> Program::add_doc(const std::string &name) {
  auto res = std::make_unique<T>(name, false);
  _on_add(*res);
  return res;
}

} // namespace logia
