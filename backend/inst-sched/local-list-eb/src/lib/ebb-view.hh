#pragma once

#include "eb-paths.hh"

class EbbView {

public:
  EbbView(const EbPaths::path_t &path) : _path(path), _size(0) {
    for (auto bb : _path)
      _size += bb->code().size();
  }

  const isa::Context &ctx() const {
    return _path.front()->parent().parent().ctx();
  }

  std::size_t size() const { return _size; }

  std::size_t offset_of(const isa::BasicBlock &bb) const {
    std::size_t res = 0;
    for (auto it : _path) {
      if (it == &bb)
        return res;
      res += it->code().size();
    }

    assert(0);
  }

  const isa::BasicBlock &bb_of(std::size_t idx) const {
    assert(idx < _size);
    auto it = _path.begin();
    while (idx >= (*it)->code().size()) {
      idx -= (*it)->code().size();
      ++it;
    }
    return **it;
  }

  const std::vector<std::string> &operator[](std::size_t idx) const {
    assert(idx < _size);
    auto it = _path.begin();
    while (idx >= (*it)->code().size()) {
      idx -= (*it)->code().size();
      ++it;
    }
    return (*it)->code()[idx];
  }

private:
  const EbPaths::path_t &_path;
  std::size_t _size;
};
