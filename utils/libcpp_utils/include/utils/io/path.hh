//===-- ioutils/path.hh - Path utils ----------------------------*- C++ -*-===//
//
// gbx-cl project
// Author: Steven Lariau
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains many util functions to manipulate paths
///
//===----------------------------------------------------------------------===//

#pragma once

#include <string>

namespace utils {
namespace path {

/// Concat 2 real/abs path in one
std::string join(const std::string &p1, const std::string &p2);

/// Cut a path in 2: first is everything before extension sign '.'
/// second is extension with sign '.' included
/// If no extension, first is path, second is empty
std::pair<std::string, std::string> split_ext(const std::string &path);

/// @returns the absolute path of `path`, relative to current working directory
std::string abspath(const std::string &path);

/// Return true if path exists and is a file
/// Follows symlink
bool is_file(const std::string &path);

/// Return true if path exists and is a file
/// Follows symlink
bool is_dir(const std::string &path);

} // namespace path
} // namespace utils
