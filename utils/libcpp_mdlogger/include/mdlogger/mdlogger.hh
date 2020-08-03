#pragma once

#include <memory>
#include <string>

#include "mddocument.hh"

class MDLogger {
public:
  MDLogger(const MDLogger &) = delete;
  MDLogger &operator=(MDLogger &) = delete;

  static MDLogger &instance();
  static void init(int argc, char **argv);

  std::unique_ptr<MDDocument> add_doc(const std::string &name);

private:
  long _timestamp;
  std::size_t _docs_count;
  std::string _dir;

  MDLogger();
};
