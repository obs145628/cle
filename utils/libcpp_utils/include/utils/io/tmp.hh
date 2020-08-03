//===-- ioutils/tmp.hh - Temporary file utils -------------------*- C++ -*-===//
//
// gbx-cl project
// Author: Steven Lariau
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Util functions to manipulate temporary files and dirs
///
//===----------------------------------------------------------------------===//

#pragma once

#include <string>

namespace utils {

/// Create a unique temporary path in /tmp folder
std::string make_tmp_path(const std::string &prefix,
                          const std::string &postfix);

/// Create a unique temporary file in /tmp with some text content `content`
/// Return the created file path
std::string make_tmp_text_file(const std::string &content,
                               const std::string &prefix,
                               const std::string &postfix);

} // namespace utils
