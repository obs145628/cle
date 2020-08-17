#include "cloner.hh"

Instruction &Cloner::clone_ins(Instruction &ins, BasicBlock &bb_pos,
                               ins_iterator_t insert_pos) {

  auto ops = _map(ins.ops());

  Instruction &res = *bb_pos.insert_ins(insert_pos, ins.get_opname(), ops, "",
                                        ins.get_def_idx());

  if (ins.has_def())
    _bb_map.emplace(&ins, &res);

  return res;
}

void Cloner::clone_bb(BasicBlock &bb, BasicBlock &dst_bb,
                      ins_iterator_t dst_pos) {
  _bb_map.clear();

  for (auto &ins : bb.ins()) {
    auto &new_ins = clone_ins(ins, dst_bb, dst_pos);
    (void)new_ins;
  }
}

std::vector<Value *> Cloner::_map(const std::vector<Value *> &vals) const {
  std::vector<Value *> res;
  for (auto v : vals)
    res.push_back(_map(v));
  return res;
}

Value *Cloner::_map(Value *val) const {
  assert(val);
  auto it = _bb_map.find(val);
  return it == _bb_map.end() ? val : it->second;
}
