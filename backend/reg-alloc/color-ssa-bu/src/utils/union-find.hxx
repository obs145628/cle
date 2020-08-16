#pragma once

#include "union-find.hh"

#include <cassert>

template <class T> void UnionFind<T>::connect(const T &a, const T &b) {
  auto id_a = _gen_id(a);
  auto id_b = _gen_id(b);

  // First get id, and check if they are the same
  auto r_a = find(id_a);
  auto r_b = find(id_b);
  if (r_a == r_b)
    return;

  // Make one id, the child of the other
  // Make the one with the fewest element the child of the other
  // It keeps the tree balanced
  if (_sizes[r_a] < _sizes[r_b]) {
    _preds[r_a] = r_b;
    _sizes[r_b] += _sizes[r_a];
  } else {
    _preds[r_b] = r_a;
    _sizes[r_a] += _sizes[r_b];
  }

  --_count;
}

template <class T> std::size_t UnionFind<T>::find(std::size_t id) const {
  while (id != _preds[id])
    id = _preds[id];
  return id;
}

template <class T> std::vector<T> UnionFind<T>::get_set(std::size_t id) const {
  // Slow linear implem
  std::vector<std::string> res;
  for (auto it : _id_map)
    if (find(it.first) == id)
      res.push_back(it.first);
  return res;
}

template <class T> void UnionFind<T>::compress() {
  std::map<T, std::size_t> new_id_map;
  std::vector<std::size_t> new_preds(items_count());
  std::size_t root_idx = 0;
  std::size_t leaf_idx = _count;

  for (std::size_t i = 0; i < items_count(); ++i) {
    if (!is_root(i))
      continue;

    auto items = get_set(i);

    new_preds[root_idx] = root_idx;
    new_id_map[items[0]] = root_idx;

    for (auto it = items.begin() + 1; it != items.end(); ++it) {
      new_preds[leaf_idx] = root_idx;
      new_id_map[*it] = leaf_idx;
      ++leaf_idx;
    }

    ++root_idx;
  }

  assert(root_idx == _count);
  assert(leaf_idx == items_count());

  _id_map = new_id_map;
  _preds = new_preds;

  // @TODO: update size
  // Not needed if doesn't reinsert in the array
}

template <class T> std::size_t UnionFind<T>::_gen_id(const T &obj) {
  auto it = _id_map.find(obj);
  if (it != _id_map.end())
    return it->second;

  auto id = _id_map.size();
  _id_map[obj] = id;
  _preds.push_back(id);
  _sizes.push_back(1);
  ++_count;
  return id;
}
