#pragma once

#include <cassert>
#include <set>

#include "iterators.hh"
#include "module.hh"

namespace isa {

template <class T> class Bitmap {
public:
  Bitmap(const Bitmap &) = default;

  explicit Bitmap(std::size_t size) : _data(0), _size(size) {
    assert(_size < 8 * sizeof(T));
  }

  void put(std::size_t pos) {
    assert(pos < _size);
    _data |= (T(1) << T(pos));
  }

  bool has(std::size_t pos) const {
    assert(pos < _size);
    return (_data >> T(pos)) & 0x1;
  }

private:
  T _data;
  std::size_t _size;

  friend bool operator==(Bitmap x, Bitmap y) {
    assert(x._size == y._size);
    return x._data == y._data;
  }
};

class op_iterator_t;
class const_op_iterator_t;

class op_iterator_t {
public:
  op_iterator_t(const op_iterator_t &) = default;
  op_iterator_t &operator=(const op_iterator_t &) = default;

  op_iterator_t(Instruction &ins, std::size_t pos, Bitmap<std::uint32_t> bmap)
      : _ins(ins), _pos(pos), _bmap(bmap) {
    assert(_pos <= ins.args.size());

    if (_pos < _ins.args.size() && !_bmap.has(_pos))
      _next();
  }

  op_iterator_t operator++() {
    _next();
    return *this;
  }

  op_iterator_t operator++(int) {
    auto res = *this;
    _next();
    return res;
  }

  op_iterator_t operator--() {
    _prev();
    return *this;
  }

  op_iterator_t operator--(int) {
    auto res = *this;
    _prev();
    return res;
  }

  std::string &operator*() const { return *_get(); }

private:
  Instruction &_ins;
  std::size_t _pos;
  Bitmap<std::uint32_t> _bmap;

  void _prev() {
    while (_pos <= _ins.args.size() && !_bmap.has(--_pos))
      ;

    assert(_pos < _ins.args.size());
  }

  void _next() {
    assert(_pos < _ins.args.size());
    ++_pos;

    while (_pos < _ins.args.size() && !_bmap.has(_pos))
      ++_pos;
  }

  std::string *_get() const {
    assert(_pos < _ins.args.size());
    return &_ins.args[_pos];
  }

  friend bool operator==(op_iterator_t x, op_iterator_t y) {
    assert(&x._ins == &y._ins && x._bmap == y._bmap);
    return x._pos == y._pos;
  }

  friend bool operator!=(op_iterator_t x, op_iterator_t y) { return !(x == y); }

  friend class const_op_iterator_t;
};

class const_op_iterator_t {
public:
  const_op_iterator_t(const const_op_iterator_t &) = default;
  const_op_iterator_t &operator=(const const_op_iterator_t &) = default;

  const_op_iterator_t(const Instruction &ins, std::size_t pos,
                      Bitmap<std::uint32_t> bmap)
      : _ins(ins), _pos(pos), _bmap(bmap) {
    assert(_pos <= ins.args.size());

    if (_pos < _ins.args.size() && !_bmap.has(_pos))
      _next();
  }

  const_op_iterator_t(op_iterator_t it)
      : const_op_iterator_t(it._ins, it._pos, it._bmap) {}

  const_op_iterator_t operator++() {
    _next();
    return *this;
  }

  const_op_iterator_t operator++(int) {
    auto res = *this;
    _next();
    return res;
  }

  const_op_iterator_t operator--() {
    _prev();
    return *this;
  }

  const_op_iterator_t operator--(int) {
    auto res = *this;
    _prev();
    return res;
  }

  const std::string &operator*() const { return *_get(); }

private:
  const Instruction &_ins;
  std::size_t _pos;
  Bitmap<std::uint32_t> _bmap;

  void _prev() {
    while (_pos <= _ins.args.size() && !_bmap.has(--_pos))
      ;

    assert(_pos < _ins.args.size());
  }

  void _next() {
    assert(_pos < _ins.args.size());
    ++_pos;

    while (_pos < _ins.args.size() && !_bmap.has(_pos))
      ++_pos;
  }

  const std::string *_get() const {
    assert(_pos < _ins.args.size());
    return &_ins.args[_pos];
  }

  friend bool operator==(const_op_iterator_t x, const_op_iterator_t y) {
    assert(&x._ins == &y._ins && x._bmap == y._bmap);
    return x._pos == y._pos;
  }

  friend bool operator!=(const_op_iterator_t x, const_op_iterator_t y) {
    return !(x == y);
  }
};

// local jump or ret
bool is_branch(const Instruction &ins);

// possible labels for branchs ins
std::vector<std::string> branch_targets(const Instruction &ins);

void dump(const Instruction &ins, std::ostream &os);

// registers read by ins
IteratorRange<op_iterator_t> uses(Instruction &ins);
IteratorRange<const_op_iterator_t> uses(const Instruction &ins);

// registers write to by ins
IteratorRange<op_iterator_t> defs(Instruction &ins);
IteratorRange<const_op_iterator_t> defs(const Instruction &ins);

} // namespace isa
