//===-- cliutils/opt.hh - Opt class definition ------------------*- C++ -*-===//
//
// gbx-cl project
// Author: Steven Lariau
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definiton of Opt class (Options)
/// Can query really simple infos about program args
///
//===----------------------------------------------------------------------===//

#include <string>
#include <vector>

namespace utils {

class Opt {

public:
  Opt(int argc, char **argv);

  bool has_flag(char sflag, const std::string &lflag = {}) const;
  bool has_flag(const std::string &lflag) const;

private:
  std::vector<std::string> _args;
};

} // namespace utils
