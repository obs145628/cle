#pragma once

#include <cassert>
#include <iterator>

template <class It> class IteratorRange {
public:
  IteratorRange(It beg, It end) : _beg(beg), _end(end) {}

  It begin() const { return _beg; }
  It end() const { return _end; }

  // std::size_t size() const { return std::distance(begin(), end()); }

  decltype(auto) front() const {
    assert(begin() != end());
    return *begin();
  }

  decltype(auto) back() const {
    assert(begin() != end());
    It it = end();
    --it;
    return *it;
  }

private:
  It _beg;
  It _end;
};
