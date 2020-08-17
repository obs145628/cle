#include "names-table.hh"

#include <cassert>

#include <utils/cli/err.hh>

namespace {

constexpr std::size_t IDX_NO = -1;

std::size_t get_idx(const std::string &val, const std::string &prefix) {
  if (prefix.size() >= val.size() || val.substr(0, prefix.size()) != prefix)
    return IDX_NO;

  auto val_idx = val.substr(prefix.size());
  std::size_t res;
  std::size_t end;
  try {
    res = std::stoll(val_idx, &end, 10);
  } catch (...) {
    return IDX_NO;
  }

  if (end != val_idx.size())
    return IDX_NO;
  return res;
}

} // namespace

NamesTable::NamesTable(const std::string &prefix) : _pref(prefix) {}

// Delete an exisiting name
// Panic if not found
void NamesTable::del(const std::string &name) {
  auto idx = get_idx(name, _pref);
  if (idx != IDX_NO) {
    PANIC_IF(idx >= _vals.size() || !_vals[idx],
             "Try to delete unknown name " + name);
    _vals[idx] = nullptr;
    return;
  }

  else {
    PANIC_IF(_custom.erase(name) != 1, "Try to delete unknown name " + name);
  }
}

std::string NamesTable::add(std::string name, Value &val) {
  if (name.empty()) {
    std::size_t idx = 0;
    while (idx < _vals.size() && _vals[idx])
      ++idx;
    if (idx == _vals.size())
      _vals.push_back(nullptr);
    name = _pref + std::to_string(idx);
  }

  auto idx = get_idx(name, _pref);

  if (idx != IDX_NO) {
    while (idx >= _vals.size())
      _vals.push_back(nullptr);
    PANIC_IF(_vals[idx], "Try to add exisiting name " + name);
    _vals[idx] = &val;
    return name;
  }

  else {
    PANIC_IF(!_custom.emplace(name, &val).second,
             "Try to add existing name " + name);
    return name;
  }
}

// Return an existing entry
// Or nullptr if not found
// Should only be used for debugging purposes
Value *NamesTable::find(const std::string &name) {
  auto idx = get_idx(name, _pref);
  if (idx != IDX_NO) {
    return idx >= _vals.size() ? nullptr : _vals[idx];
  }

  else {
    auto it = _custom.find(name);
    return it == _custom.end() ? nullptr : it->second;
  }
}
