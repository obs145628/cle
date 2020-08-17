#pragma once

#include "module.hh"

class Cloner {

public:
  Instruction &clone_ins(Instruction &ins, BasicBlock &bb_pos,
                         ins_iterator_t insert_pos);

  void clone_bb(BasicBlock &bb, BasicBlock &dst_bb, ins_iterator_t dst_pos);

private:
  std::map<Value *, Value *> _bb_map;

  std::vector<Value *> _map(const std::vector<Value *> &vals) const;
  Value *_map(Value *val) const;
};
