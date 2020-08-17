#pragma once

#include <cassert>
#include <iterator>
#include <type_traits>

template <class It, class FMap> class IteratorAdapterMap {

private:
  It _it;
  FMap _fmap;

  using ret_type = decltype(_fmap(*_it));

public:
  IteratorAdapterMap(const It &it, const FMap &fmap) : _it(it), _fmap(fmap) {}

  IteratorAdapterMap operator++() {
    ++_it;
    return *this;
  }

  IteratorAdapterMap operator++(int) {
    auto res = *this;
    ++_it;
    return res;
  }

  IteratorAdapterMap operator--() {
    --_it;
    return *this;
  }

  IteratorAdapterMap operator--(int) {
    auto res = *this;
    --_it;
    return res;
  }

  ret_type operator*() const { return _fmap(*_it); }

  friend bool operator==(const IteratorAdapterMap &x,
                         const IteratorAdapterMap &y) {
    return x._it == y._it;
  }

  friend bool operator!=(const IteratorAdapterMap &x,
                         const IteratorAdapterMap &y) {
    return x._it != y._it;
  }
};

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

  template <class F> IteratorRange<IteratorAdapterMap<It, F>> map(F f) {
    auto beg = IteratorAdapterMap<It, F>(_beg, f);
    auto end = IteratorAdapterMap<It, F>(_end, f);
    return IteratorRange<IteratorAdapterMap<It, F>>(beg, end);
  }

private:
  It _beg;
  It _end;
};
