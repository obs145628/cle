#pragma once

#include <map>
#include <string>

// Configuration file
// list of string Key-value pair
class MDConfig {

public:
  void put(const std::string &key, const std::string &val);

  void write_to_file(const std::string &path) const;

private:
  std::map<std::string, std::string> _dict;
};
