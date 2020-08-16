#include <logia/doc-config.hh>

#include <cassert>
#include <fstream>

namespace logia {

void DocConfig::write_to_file(const std::string &path) const {
  std::ofstream os(path);
  assert(os.good());

  os << "id = " << id << "\n";
  os << "link-in-root = " << (link_in_root ? "true" : "false") << "\n";
  os << "name = " << name << "\n";
  os << "time = " << time << "\n";
  os << "type = " << type << "\n";
  assert(os.good());
}

} // namespace logia
