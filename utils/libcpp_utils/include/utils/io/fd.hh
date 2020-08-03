//===-- ioutils/fd.hh - File descriptor utils -------------------*- C++ -*-===//
//
// gbx-cl project
// Author: Steven Lariau
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Util functions to manipulate files using file descriptors
///
//===----------------------------------------------------------------------===//

#pragma once

#include <string>

namespace utils {

/// Return the full content available from `fd` in one string
std::string fd_read_all_str(int fd);

} // namespace utils
