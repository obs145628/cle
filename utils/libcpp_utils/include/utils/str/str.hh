#pragma once

#include <string>
#include <vector>

namespace utils {

namespace str {

std::string trim(const std::string &str);

std::vector<std::string> split(const std::string &str, char sep);

long parse_long(const std::string &str);

bool begins_with(const std::string &str, const std::string &pre);
bool ends_with(const std::string &str, const std::string &post);

std::string replace(const std::string &str, char pat, const std::string &rep);

} // namespace str
} // namespace utils
