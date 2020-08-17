#pragma once

#include <cassert>
#include <map>
#include <vector>

// Adapter to convert list of N objects of type T into size_t from [0 to N - 1]
// Can convert back and forth between representations
template <class T> class VertexAdapter {

public:
  template <class Container>
  VertexAdapter(const Container &container)
      : VertexAdapter(std::begin(container), std::end(container)) {}

  template <class It>
  VertexAdapter(It begin, It end)
      : _v2o(_build_v2o(begin, end)), _o2v(_build_o2v(_v2o)) {}

  T v2o(std::size_t v) const {
    assert(v < _v2o.size());
    return _v2o[v];
  }

  std::size_t o2v(T o) const {
    auto it = _o2v.find(o);
    assert(it != _o2v.end());
    return it->second;
  }

  std::size_t size() const { return _v2o.size(); }

  T operator()(std::size_t v) const { return v2o(v); }
  std::size_t operator()(T o) const { return o2v(o); }

private:
  const std::vector<T> _v2o;
  const std::map<T, std::size_t> _o2v;

  template <class It> static std::vector<T> _build_v2o(It begin, It end) {
    std::vector<T> res;
    for (auto it = begin; it != end; ++it)
      res.push_back(*it);
    return res;
  }

  static std::map<T, std::size_t> _build_o2v(const std::vector<T> &v2o) {
    std::map<T, std::size_t> res;
    for (std::size_t i = 0; i < v2o.size(); ++i)
      res.emplace(v2o[i], i);
    return res;
  }
};
