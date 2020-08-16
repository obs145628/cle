#pragma once

#include <map>
#include <string>

namespace logia {

// Configuration file for a Document
struct DocConfig {
  // Unique document identifier
  std::string id;

  // If true, doc will be visible in the root document
  bool link_in_root;

  // Document title
  std::string name;

  // timestamp in milliseconds
  unsigned long time;

  // document type (markdown, etc)
  std::string type;

  void write_to_file(const std::string &path) const;
};

} // namespace logia
