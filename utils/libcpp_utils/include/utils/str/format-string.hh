//===-- strutils/format-string.hh - string format tools ---------*- C++ -*-===//
//
// gbx-cl project
// Author: Steven Lariau
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains define an internal helper class and a maccro to create
/// formated string
///
//===----------------------------------------------------------------------===//

#pragma once

#include <sstream>

namespace gbx_macros_funs {

/// Utils class to create a std::string, formatted the same way than streams
/// Shouldn't be used directly
/// Use the maccro FORMAT_STRING instead
class StringFormatter {
public:
  StringFormatter() = default;
  StringFormatter(const StringFormatter &) = delete;

  template <class T> StringFormatter &operator<<(const T &x) {
    _os << x;
    return *this;
  }

  std::string str() { return _os.str(); }

private:
  std::ostringstream _os;
};

} // namespace gbx_macros_funs

/// Maccro to create std::string with stream formatting (<<), in one line
/// @param CODE stream-like format expression
/// @returns formatted script
#define FORMAT_STRING(CODE)                                                    \
  (((::gbx_macros_funs::StringFormatter() << CODE)).str())

/// Format using only ostringstream
/// It seems to solve bug where overloads for ostream& are not seen by
/// StringFormatter
#define FMT_OSS(CODE)                                                          \
  (dynamic_cast<std::ostringstream *>(&(std::ostringstream() << CODE))->str())
