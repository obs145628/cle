#pragma once

#include <map>
#include <vector>

template <class T> class UnionFind {

public:
  UnionFind() : _count(0) {}

  void connect(const T &a, const T &b);

  std::size_t find(const T &a) const { return find(_id_map.at(a)); }
  std::size_t find(std::size_t id) const;
  bool connected(const T &a, const T &b) const { return find(a) == find(b); }

  std::size_t items_count() const { return _id_map.size(); }
  std::size_t sets_count() const { return _count; }

  std::vector<T> get_set(std::size_t id) const;

  bool is_root(std::size_t id) const { return find(id) == id; }
  bool is_root(const T &a) const { return is_root(_id_map.at(a)); }

  void compress();

private:
  std::size_t _count;
  std::map<T, std::size_t> _id_map;
  std::vector<std::size_t> _preds;
  std::vector<std::size_t> _sizes;

  std::size_t _gen_id(const T &obj);
};

#include "union-find.hxx"
