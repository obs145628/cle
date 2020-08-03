//===-- ioutils/file.hh - File utils ----------------------------*- C++ -*-===//
//
// gbx-cl project
// Author: Steven Lariau
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Util functions to manipulate files
///
//===----------------------------------------------------------------------===//

#pragma once

#include <string>

namespace utils {

/// Store `content` into a new file `path`
void write_file_str(const std::string &path, const std::string &content);

/// Return a full text-file into one string
std::string read_file_str(const std::string &path);

} // namespace utils
