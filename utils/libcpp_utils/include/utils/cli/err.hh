//===-- cliutils/err.hh - error utils ---------------------------*- C++ -*-===//
//
// gbx-cl project
// Author: Steven Lariau
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Many util functions for error handling
///
//===----------------------------------------------------------------------===//

#pragma once

#include <string>

namespace gbx_macros_funs {

[[noreturn]] void __panic_fn(const char *file, const char *fun, int line,
                             const std::string &mess);

}

/// Abort the current program with error `Mess`
#define PANIC(Mess)                                                            \
  (::gbx_macros_funs::__panic_fn(__FILE__, __func__, __LINE__, Mess))

/// Abort the current program with error `Mess` if `Cond` is true
#define PANIC_IF(Cond, Mess)                                                   \
  do {                                                                         \
    if (static_cast<bool>(Cond))                                               \
      PANIC(Mess);                                                             \
  } while (0)
