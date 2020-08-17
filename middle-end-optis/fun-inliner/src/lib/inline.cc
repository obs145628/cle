#include "inline.hh"

#include <set>

#include "copy.hh"
#include <utils/cli/err.hh>

namespace {

void find_regs(const Instruction &ins, std::set<std::string> &out_args) {
  for (const auto &arg : ins.args)
    if (arg[0] == '%')
      out_args.insert(arg);
}

} // namespace

void inline_call(Instruction &call_site) {
  // Get call site informations
  PANIC_IF(call_site.args[0] != "call", "invalid ins");

  std::string ret_reg;
  if (call_site.args[1][0] != '@') {
    ret_reg = call_site.args[1];
    PANIC_IF(ret_reg[0] != '%', "Invalid ret register");
  }
  bool has_ret = !ret_reg.empty();

  std::string callee_name = call_site.args[1 + has_ret].substr(1);
  Function &caller = call_site.parent().parent();
  const Module &mod = caller.parent();
  PANIC_IF(mod.get_fun(callee_name) == nullptr, "Undefined function");
  const Function &callee = *mod.get_fun(callee_name);

  std::vector<std::string> caller_args(call_site.args.begin() + 2 + has_ret,
                                       call_site.args.end());
  const auto &callee_args = callee.args();
  PANIC_IF(caller_args.size() != callee_args.size(),
           "Invalid number of arguments");
  BasicBlock &bb_call = call_site.parent();
  bool bb_call_is_entry =
      caller.has_entry_bb() && &caller.get_entry_bb() == &bb_call;

  // Find and rename all regs in callee
  // This is to avoid register name clash
  CloneMap cm;
  std::set<std::string> regs;
  static int inline_count = 0;
  std::string reg_prefix = "%il_" + std::to_string(inline_count++) + "_";
  for (const auto &bb : callee.bb())
    for (const auto &ins : bb.ins())
      find_regs(ins, regs);
  for (const auto &r : regs)
    cm.regs[r] = reg_prefix + r.substr(1);

  // Rename args in callee by actual values in caller
  for (std::size_t i = 0; i < caller_args.size(); ++i)
    cm.regs[callee_args[i]] = caller_args[i];

  // Clone BBs of callee before caller bb
  // assumes callee entry is first BB
  auto cloned_bb =
      clone_fun_into(callee, caller, bb_iterator_t(&bb_call), cm, true);

  // Divide caller bb in 2:
  // - before call, all ins before call, + jump to cloned_bb
  // - after call, all ins after call
  // After this, caller bb not needed anymore an can be erased
  // We get BBs in order {bb_before, cloned bbs, bb_after} at the location of
  // bb_call
  // Also need to make before_call the entry if the caller bb was the entry
  std::string bb_call_label = bb_call.label();
  BasicBlock &bb_before = *caller.insert_bb(cloned_bb);
  BasicBlock &bb_after = *caller.insert_bb(bb_iterator_t(&bb_call));
  BasicBlock::ins_move(bb_call, bb_call.ins_begin(), ins_iterator_t(&call_site),
                       bb_before, bb_before.ins_begin());
  BasicBlock::ins_move(bb_call, ins_iterator_t(&call_site) + 1,
                       bb_call.ins_end(), bb_after, bb_after.ins_begin());
  bb_before.insert_ins(bb_before.ins_end(), {"b", "@" + cloned_bb->label()});
  if (bb_call_is_entry)
    caller.set_entry_bb(bb_before);
  caller.erase_bb(bb_call);

  // Replace all uses of old call_bb by new bb_before
  replace_all_ops("@" + bb_call_label, "@" + bb_before.label(), caller);

  // Replace all ret instructions in cloned BBs by a mov into ret reg + branch
  // to bb_after
  for (auto it = cloned_bb; it != bb_iterator_t(&bb_after); ++it) {
    auto &bb = *it;
    auto &bins = bb.ins().back();

    if (bins.args[0] == "ret") {
      if (has_ret) {
        PANIC_IF(bins.args.size() != 2, "ret must return a value");
        bins.args = {"mov", ret_reg, bins.args[1]};
        bb.insert_ins(bb.ins_end(), {"b", "@" + bb_after.label()});
      } else {
        bins.args = {"b", "@" + bb_after.label()};
      }
    }
  }
}
