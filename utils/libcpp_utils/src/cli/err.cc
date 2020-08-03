#include <utils/cli/err.hh>

#include <exception>
#include <iostream>

namespace gbx_macros_funs {

void __panic_fn(const char *file, const char *fun, int line,
                const std::string &mess) {
  std::cerr << "Panic at " << file << ":" << line << " (" << fun << "): `"
            << mess << "'\n";
  std::terminate();
}

} // namespace gbx_macros_funs
