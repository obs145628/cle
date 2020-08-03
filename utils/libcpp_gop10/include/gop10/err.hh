#pragma once

#include <stdexcept>
#include <string>

#include <utils/str/format-string.hh>

namespace gop {

class RTError : public std::exception {

public:
  RTError(const std::string &msg) : _msg(msg) {}

  const char *what() const throw() { return _msg.c_str(); }

private:
  std::string _msg;
};

inline void throw_rt_err(const std::string &msg, bool cond = true) {
  if (cond)
    throw RTError(msg);
}

} // namespace gop

#define GOP_ERR_IF(Cond, Mess) (::gop::throw_rt_err(FMT_OSS(Mess), Cond))
#define GOP_ERR(Mess) (GOP_ERR_IF(true, Mess))
