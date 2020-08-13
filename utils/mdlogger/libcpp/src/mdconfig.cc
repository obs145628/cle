#include <mdlogger/mdconfig.hh>

#include <cassert>
#include <fstream>

void MDConfig::put(const std::string &key, const std::string &val) {
  assert(_dict.count(key) == 0);
  _dict.emplace(key, val);
}

void MDConfig::write_to_file(const std::string &path) const {
  std::ofstream os(path);
  assert(os.good());
  for (const auto &it : _dict)
    os << it.first << " = " << it.second << "\n";
  assert(os.good());
}
