#pragma once

#include <map>
#include <string>
#include <vector>

class Value;

class NamesTable {

public:
  // Table used to ensure a set of values have unique names
  // Shouldn't be used for value retrieval from key
  // prefix is default prefix given when generating names
  // Manipulation of names with this prefix is faster
  // But custom names can also be used
  NamesTable(const std::string &prefix);

  // Delete an exisiting name
  // Panic if not found
  void del(const std::string &name);

  // Add a new entry
  // Generate a new name if name is empty
  // Panic if already taken
  // Return its name
  std::string add(std::string name, Value &val);

  // Return an existing entry
  // Or nullptr if not found
  // Should only be used for debugging purposes
  Value *find(const std::string &name);

private:
  std::string _pref;
  std::vector<Value *> _vals;
  std::map<std::string, Value *> _custom;
};
