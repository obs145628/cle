#include "value.hh"

#include <cassert>

#include "../isa/isa.hh"
#include "module.hh"
#include "names-table.hh"

#include <iostream>

namespace {

// Value is not used anymore
// Only destroy const / argfunction
// Fun / BBs / Ins are not destroyed because:
// 1: they are node of ptr_list, which is responsible for alloc / dealloc
// 2: It's useless but not forbidden to have fun / ins / bb unused
void destroy(Value *ptr) {
  if (dynamic_cast<ValueConst *>(ptr) || dynamic_cast<ValueArg *>(ptr))
    delete ptr;
}

} // namespace

void ValueUser::reset(Value *new_val) {
  if (_val == new_val)
    return;

  if (_val) {
    auto it = _val->_users.find(&_user);
    assert(it != _val->_users.end() && it->second > 0);
    if (--it->second == 0)
      _val->_users.erase(it);
    // std::cout << "del {" << _val->to_arg() << "}: use = " <<
    // _val->_users.size()
    //        << "\n";
    if (_val->_users.empty() && !_val->_owned)
      destroy(_val);
  }
  _val = new_val;
  if (_val) {
    _val->_users.emplace(&_user, 0);
    ++_val->_users[&_user];
  }
}

void ValueOwner::reset(Value *new_val) {
  if (_val == new_val)
    return;

  if (_val)
    destroy(_val);
  _val = new_val;
  if (new_val) {
    assert(!new_val->_owned);
    new_val->_owned = true;
  }
}

Value::Value(const std::vector<Value *> &ops, NamesTable &ntable,
             const std::string &name)
    : _owned(false), _ntable(ntable) {
  for (auto op : ops)
    _ops.emplace_back(*this, op);
  _name = _ntable.add(name, *this);
}

const std::string &Value::get_name() const { return _name; }

void Value::set_name(const std::string &new_name) {
  _ntable.del(_name);
  _name = _ntable.add(new_name, *this);
}

Value &Value::op(std::size_t idx) {
  assert(idx < _ops.size());
  assert(_ops[idx]._val);
  return *_ops[idx]._val;
}

const Value &Value::op(std::size_t idx) const {
  assert(idx < _ops.size());
  assert(_ops[idx]._val);
  return *_ops[idx]._val;
}

Value::~Value() {
  _ntable.del(_name);
  if (_users.empty())
    return;

  // Should only be there if Value is a Fun / BB / Ins that is still in use
  // Or if the object was owned and the owner is destroyed
  // assert(_owned || dynamic_cast<Instruction *>(this) ||
  //       dynamic_cast<BasicBlock *>(this) || dynamic_cast<Function *>(this));

  // 3 solutions:
  // - Abort because program in inconcistent state
  //  (maybe some transforms require to go through an inconcistent state ?)
  // - Does nothing, and says it's UB to try to use this pointer
  //  (not possible with current implem because of ~ValueUser that will deref
  //  this ptr)
  // - Replace all ptr value of users with nullptr
  //   (this way easier to debug use of this deleted ins / bbs)
  //

  // Replace all uses of this with nullptr
  for (auto it : _users) {
    auto u = it.first;
    for (std::size_t i = 0; i < u->_ops.size(); ++i)
      if (u->_ops[i]._val == this)
        u->_ops[i]._val = nullptr;
  }
}

std::vector<Value *> Value::ops() {
  std::vector<Value *> res;
  for (const auto &op : _ops)
    res.push_back(op._val);
  return res;
}

std::vector<const Value *> Value::ops() const {
  std::vector<const Value *> res;
  for (const auto &op : _ops)
    res.push_back(op._val);
  return res;
}

Value &Value::set_op(std::size_t idx, Value &val) {
  assert(idx < _ops.size());
  _ops[idx].reset(&val);
  return val;
}

namespace {

NamesTable g_ntable_const("g");

}

ValueConst *ValueConst::make(long val, NamesTable &ntable,
                             const std::string &name) {
  return new ValueConst(val, ntable, name);
}

ValueConst *ValueConst::make(long val, const std::string &name) {
  return ValueConst::make(val, g_ntable_const, name);
}

std::string ValueConst::to_arg() const { return std::to_string(_val); }

void ValueConst::dump(std::ostream &os) const { os << "Const(" << _val << ")"; }

ValueArg *ValueArg::make(const Function &fun, std::size_t pos,
                         NamesTable &ntable, const std::string &name) {
  return new ValueArg(fun, pos, ntable, name);
}

ValueArg::ValueArg(const Function &fun, std::size_t pos, NamesTable &ntable,
                   const std::string &name)
    : Value({}, ntable, name), _fun(fun), _pos(pos) {
  assert(_pos < isa::fundecl_args_count(fun.decl()));
}

std::string ValueArg::to_arg() const {
  return "%" + isa::fundecl_args(_fun.decl())[_pos];
}

void ValueArg::dump(std::ostream &os) const {
  os << "Arg(" << isa::fundecl_args(_fun.decl())[_pos] << ":" << _fun.get_name()
     << ":#" << _pos << ")";
}
