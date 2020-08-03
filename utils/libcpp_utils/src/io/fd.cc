#include <utils/io/fd.hh>

#include <unistd.h>

namespace utils {

std::string fd_read_all_str(int fd) {
  std::string res;
  char buf[BUFSIZ];

  for (;;) {
    ssize_t nread = read(fd, buf, sizeof(buf));
    if (nread <= 0)
      break;

    res.insert(res.end(), buf, buf + nread);
  }

  return res;
}

} // namespace utils
