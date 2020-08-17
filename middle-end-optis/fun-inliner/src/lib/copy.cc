#include "copy.hh"

#include <iostream>

namespace {

bool transform_op(std::string &op,
                  const std::map<std::string, std::string> &map) {
  auto it = map.find(op);
  if (it == map.end())
    return false;

  op = it->second;
  return true;
}

} // namespace

void clone_ins(const Instruction &ins, const CloneMap &cm, BasicBlock &out_bb,
               ins_iterator_t insert_point) {
  const auto &fun = ins.parent().parent();
  auto new_args = ins.args;

  for (std::size_t i = 1; i < new_args.size(); ++i) {
    auto &arg = new_args[i];
    if (arg[0] == '%')
      transform_op(arg, cm.regs);
    else if (arg[0] == '@') {
      if (!fun.get_bb(arg.substr(1)) || !transform_op(arg, cm.bbs))
        transform_op(arg, cm.funs);
    }
  }

  out_bb.insert_ins(insert_point, new_args);
}

void clone_bb_into(const BasicBlock &bb, BasicBlock &out_bb,
                   ins_iterator_t insert_point, const CloneMap &cm) {
  for (const auto &ins : bb.ins())
    clone_ins(ins, cm, out_bb, insert_point);
}

bb_iterator_t clone_fun_into(const Function &fun, Function &out_fun,
                             bb_iterator_t out_it, const CloneMap &cm,
                             bool remap_new_bbs) {
  // Create new bbs and register labels
  std::vector<BasicBlock *> new_bbs;
  CloneMap full_cm = cm;
  for (auto &bb : fun.bb()) {
    auto &new_bb = *out_fun.insert_bb(out_it);
    new_bbs.push_back(&new_bb);

    if (remap_new_bbs) {
      full_cm.bbs.emplace("@" + bb.label(), "@" + new_bb.label());
    }
  }

  // Perform cloning of the BBs
  std::size_t i = 0;
  for (auto &bb : fun.bb()) {
    auto out_bb = new_bbs[i++];
    clone_bb_into(bb, *out_bb, out_bb->ins_begin(), full_cm);
  }

  return bb_iterator_t(new_bbs[0]);
}

void replace_all_ops(const std::string &old_val, const std::string &new_val,
                     BasicBlock &bb) {
  for (auto &ins : bb.ins())
    for (auto &arg : ins.args)
      if (arg == old_val)
        arg = new_val;
}

void replace_all_ops(const std::string &old_val, const std::string &new_val,
                     Function &fun) {
  for (auto &bb : fun.bb())
    replace_all_ops(old_val, new_val, bb);
}

void replace_all_ops(const std::string &old_val, const std::string &new_val,
                     Module &mod) {
  for (auto &fun : mod.fun())
    replace_all_ops(old_val, new_val, fun);
}
