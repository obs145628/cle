#pragma once

#include <cassert>
#include <cstddef>
#include <set>
#include <utility>
#include <vector>

template <class T> class PtrListNode;

template <class T> class PtrList;

template <class T> class PtrListNode {
public:
  PtrListNode() = default;
  PtrListNode(const PtrListNode &) = delete;
  PtrListNode &operator=(const PtrListNode &) = delete;

private:
  T *_plist_prev;
  T *_plist_next;

  friend class PtrList<T>;
};

// List of ptrs implemented as a doubly linked-list with sentinel
// All pointers are allocated and free internally as soon as item removed
template <class T> class PtrList {

public:
  class iterator_t;
  class const_iterator_t;

  class iterator_t {

  public:
    iterator_t(const iterator_t &) = default;
    iterator_t &operator=(const iterator_t &) = default;

    explicit iterator_t(T *ptr) : _ptr(ptr) {}

    iterator_t operator++() {
      _next();
      return *this;
    }

    iterator_t operator++(int) {
      auto res = *this;
      _next();
      return res;
    }

    iterator_t operator--() {
      _prev();
      return *this;
    }

    iterator_t operator--(int) {
      auto res = *this;
      _prev();
      return res;
    }

    // Really usefull, but slow implementation
    iterator_t operator+(std::size_t dist) const {
      auto res = *this;
      for (std::size_t i = 0; i < dist; ++i)
        ++res;
      return res;
    }

    iterator_t operator-(std::size_t dist) const {
      auto res = *this;
      for (std::size_t i = 0; i < dist; ++i)
        --res;
      return res;
    }

    T &operator*() const { return *_get(); }

    T *operator->() const { return _get(); }

  private:
    T *_ptr;

    void _prev() {
      assert(_ptr->_plist_prev);
      _ptr = _ptr->_plist_prev;
    }

    void _next() {
      assert(!_sentinel());
      _ptr = _ptr->_plist_next;
    }

    T *_get() const {
      assert(!_sentinel());
      return _ptr;
    }

    bool _sentinel() const { return _ptr->_plist_next == nullptr; }

    friend bool operator==(PtrList<T>::iterator_t x, PtrList<T>::iterator_t y) {
      return x._ptr == y._ptr;
    }

    friend bool operator!=(PtrList<T>::iterator_t x, PtrList<T>::iterator_t y) {
      return x._ptr != y._ptr;
    }

    friend class PtrList;
  };

  class const_iterator_t {

  public:
    const_iterator_t(const const_iterator_t &) = default;
    const_iterator_t &operator=(const const_iterator_t &) = default;

    explicit const_iterator_t(const T *ptr) : _ptr(ptr) {}

    const_iterator_t(iterator_t it) : _ptr(it._ptr) {}

    const_iterator_t operator++() {
      _next();
      return *this;
    }

    const_iterator_t operator++(int) {
      auto res = *this;
      _next();
      return res;
    }

    const_iterator_t operator--() {
      _prev();
      return *this;
    }

    const_iterator_t operator--(int) {
      auto res = *this;
      _prev();
      return res;
    }

    // Really usefull, but slow implementation
    const_iterator_t operator+(std::size_t dist) const {
      auto res = *this;
      for (std::size_t i = 0; i < dist; ++i)
        ++res;
      return res;
    }

    const_iterator_t operator-(std::size_t dist) const {
      auto res = *this;
      for (std::size_t i = 0; i < dist; ++i)
        --res;
      return res;
    }

    const T &operator*() const { return *_get(); }

    const T *operator->() const { return _get(); }

  private:
    const T *_ptr;

    void _prev() {
      assert(_ptr->_plist_prev);
      _ptr = _ptr->_plist_prev;
    }

    void _next() {
      assert(!_sentinel());
      _ptr = _ptr->_plist_next;
    }

    const T *_get() const {
      assert(!_sentinel());
      return _ptr;
    }

    bool _sentinel() const { return _ptr->_plist_next == nullptr; }

    friend bool operator==(PtrList<T>::const_iterator_t x,
                           PtrList<T>::const_iterator_t y) {
      return x._ptr == y._ptr;
    }

    friend bool operator!=(PtrList<T>::const_iterator_t x,
                           PtrList<T>::const_iterator_t y) {
      return x._ptr != y._ptr;
    }

    friend class PtrList;
  };

  static constexpr std::size_t INDEX_NONE = -1;

  // Create empty list
  PtrList() : _head(_new_sentinel()), _sentinel(_head) {}

  ~PtrList() {
    clear();
    _delete_sentinel(_sentinel);
  }

  iterator_t begin() { return iterator_t(_head); }
  const_iterator_t begin() const { return const_iterator_t(_head); }
  const_iterator_t cbegin() const { return const_iterator_t(_head); }
  iterator_t end() { return iterator_t(_sentinel); }
  const_iterator_t end() const { return const_iterator_t(_sentinel); }
  const_iterator_t cend() const { return const_iterator_t(_sentinel); }

  // Remove all items in list
  void clear() {
    T *ptr = _head;
    while (ptr->_plist_next) {
      T *next = ptr->_plist_next;
      _delete(ptr);
      ptr = next;
    }
    _head = _sentinel;
  }

  // constructor and insert item in place at position `it`
  // returns iter to newly inserted element
  template <class... Args> iterator_t emplace(iterator_t it, Args &&... args) {
    _check_it(it);
    T *item = it._ptr;
    T *prev = item->_plist_prev;

    T *new_item = _new(std::forward<Args>(args)...);
    new_item->_plist_prev = prev;
    new_item->_plist_next = item;

    if (!prev) // root
      _head = new_item;
    else
      prev->_plist_next = new_item;
    item->_plist_prev = new_item;

    return iterator_t(new_item);
  }

  // delete item at position iter
  // return iter to element right after
  iterator_t erase(iterator_t it) {
    _check_it(it);
    T *item = it._ptr;
    assert(item != _sentinel);
    T *prev = item->_plist_prev;
    T *next = item->_plist_next;

    _delete(item);
    if (!prev) // root
      _head = next;
    else
      prev->_plist_next = next;
    next->_plist_prev = prev;
    return iterator_t(next);
  }

  // move all items in range [in_beg, in_end] to list out_list, at  out_beg
  // return iterator pointing to first inserted item in out_list
  iterator_t move(iterator_t in_beg, iterator_t in_end, PtrList &out_list,
                  iterator_t out_beg) {
    _check_it(in_beg);
    _check_it(in_end);
    out_list._check_it(out_beg);

    if (in_beg == in_end)
      return out_beg;

    T *in_front = in_beg._ptr;
    T *in_prev = in_front->_plist_prev;
    T *in_next = in_end._ptr;
    T *in_back = in_next->_plist_prev;

    T *out_next = out_beg._ptr;
    T *out_prev = out_next->_plist_prev;

    // Fix input list
    if (!in_prev) // root
      _head = in_next;
    else
      in_prev->_plist_next = in_next;
    in_next->_plist_prev = in_prev;

    // Fix output list
    if (!out_prev) // root
      out_list._head = in_front;
    else
      out_prev->_plist_next = in_front;
    out_next->_plist_prev = in_back;
    in_front->_plist_prev = out_prev;
    in_back->_plist_next = out_next;

    return in_beg;
  }

  // Return index of item in list, or INDEX_NONE if not in list
  std::size_t index_of(const_iterator_t it) const {
    auto ptr = it._ptr;
    std::size_t res = 0;

    while (ptr->_plist_prev) {
      ptr = ptr->_plist_prev;
      ++res;
    }

    return ptr == _head ? res : INDEX_NONE;
  }

  // reorder all the items in the list
  void reorder(const std::vector<T *> &new_order) {
    // Check if new_order contain all items, no more
    assert(new_order.size() == index_of(const_iterator_t(_sentinel)));
    std::set<const T *> items;
    for (auto it = begin(); it != end(); ++it)
      items.insert(&*it);
    for (auto it : new_order)
      items.erase(it);
    assert(items.empty());

    // nothing to do if empty list
    if (new_order.empty())
      return;

    // reorder items
    for (std::size_t i = 0; i < new_order.size(); ++i) {
      T *item = new_order[i];
      item->_plist_prev = i ? new_order[i - 1] : nullptr;
      item->_plist_next =
          i + 1 < new_order.size() ? new_order[i + 1] : _sentinel;
    }
    _head = new_order[0];
  }

private:
  T *_head;
  T *_sentinel;

  template <class... Args> T *_new(Args &&... args) {
    return new T(std::forward<Args>(args)...);
  }

  void _delete(T *ptr) { delete ptr; }

  T *_new_sentinel() {
    // @EXTRA: I Suppode this is UB
    // T constructor not called, but shouldn't be a problem because only
    // prev/next are accessed, and they are init
    T *ptr = reinterpret_cast<T *>(new char[sizeof(T)]);
    ptr->_plist_prev = nullptr;
    ptr->_plist_next = nullptr;
    return ptr;
  }

  void _delete_sentinel(T *ptr) { delete[] reinterpret_cast<char *>(ptr); }

  void _check_it(const_iterator_t it) { assert(index_of(it) != INDEX_NONE); }
};
