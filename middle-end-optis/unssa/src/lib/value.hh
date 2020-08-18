#pragma once

#include <map>
#include <ostream>
#include <string>
#include <vector>

class Value;
class ValueUser;
class ValueOwner;

class ValueConst;
class ValueArg;

class NamesTable;

// Class to represent the usage of a Value by another one
// To make sure destructor is called when value is not used anymore
class ValueUser {

public:
  ValueUser(Value &user, Value *val = nullptr) : _user(user), _val(nullptr) {
    reset(val);
  }
  ValueUser(const ValueUser &) = delete;
  ValueUser(ValueUser &&v) : _user(v._user), _val(v._val) { v._val = nullptr; }

  ~ValueUser() { reset(nullptr); }

  // Change the value currently being used
  void reset(Value *new_val);

private:
  Value &_user;
  Value *_val;

  friend class Value;
};

// Class to represent a value whole lifecycle is linked to this object
// The value get destroyed as soon at this one is destroyed, even if they are
// some users left
class ValueOwner {

public:
  ValueOwner(Value *val) : _val(nullptr) { reset(val); }
  ValueOwner(const ValueOwner &) = delete;
  ValueOwner(ValueOwner &&v) : _val(v._val) { v._val = nullptr; }
  ~ValueOwner() { reset(nullptr); }

  // Change the value currently being owned
  void reset(Value *new_val);

  Value *get() const { return _val; }

private:
  Value *_val;
};

// Represent a value for any instruction operand
// Can be a:
// - ValueConst (int constant)
// - ValueArg (function argument)
// - Instruction (Refers to the register where the ins result is stored, only
//   valid is ins has a def)
// - BasicBlock (Used by branching instruction to refer to branch target)
// - Function (Used for call ins, or to take fun address)
class Value {

public:
  Value(const std::vector<Value *> &ops, NamesTable &ntable,
        const std::string &name);
  Value(const Value &) = delete;
  Value(Value &&) = delete;
  virtual ~Value();

  // Return unique name identifier
  const std::string &get_name() const;

  // Change unique name identifier
  // If empty, generate a new name
  void set_name(const std::string &new_name);

  std::vector<Value *> ops();
  std::vector<const Value *> ops() const;

  std::size_t ops_count() const { return _ops.size(); }
  Value &op(std::size_t idx);
  const Value &op(std::size_t idx) const;

  // Replace operand at position idx with another one
  Value &set_op(std::size_t idx, Value &val);

  // Convert value to string argument valid for gop::Ins types
  virtual std::string to_arg() const = 0;

  virtual void dump(std::ostream &os) const = 0;

private:
  std::vector<ValueUser> _ops;
  std::map<Value *, int> _users;
  bool _owned;
  NamesTable &_ntable;
  std::string _name;

  friend class ValueUser;
  friend class ValueOwner;
};

// Constant integer
class ValueConst : public Value {

public:
  // Must be passed to a ValueUser / ValueOwner to avoid memleak
  static ValueConst *make(long val, NamesTable &ntable,
                          const std::string &name);
  static ValueConst *make(long val, const std::string &name = {});

  std::string to_arg() const override;

  void dump(std::ostream &os) const override;

private:
  const long _val;

  ValueConst(long val, NamesTable &ntable, const std::string &name)
      : Value({}, ntable, name), _val(val) {}
};

class Function;

// Register for function argument
class ValueArg : public Value {

public:
  // Must be passed to a ValueUser / ValueOwner to avoid memleak
  static ValueArg *make(const Function &fun, std::size_t pos,
                        NamesTable &ntable, const std::string &name);

  std::string to_arg() const override;

  void dump(std::ostream &os) const override;

private:
  const Function &_fun;
  std::size_t _pos;

  ValueArg(const Function &fun, std::size_t pos, NamesTable &ntable,
           const std::string &name);
};
