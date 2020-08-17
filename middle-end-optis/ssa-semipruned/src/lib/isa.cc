#include "isa.hh"

#include <algorithm>
#include <cassert>

#include "gop.hh"

namespace isa {

namespace {

std::vector<std::string> branch_ins = {{"b", "blt", "ret"}};

std::vector<std::string> uses_only_ins = {{"blt"}};

std::vector<std::size_t> list_regs(const Instruction &ins) {
  std::vector<std::size_t> res;
  for (std::size_t i = 0; i < ins.args.size(); ++i) {
    const auto &arg = ins.args[i];
    if (arg[0] == '%')
      res.push_back(i);
  }
  return res;
}

std::vector<std::string> list_labels(const Instruction &ins) {
  std::vector<std::string> res;
  for (const auto &arg : ins.args)
    if (arg[0] == '@')
      res.push_back(arg.substr(1));
  return res;
}

std::vector<std::size_t> list_uses(const Instruction &ins) {
  auto regs = list_regs(ins);
  if (regs.empty())
    return {};

  bool no_defs = std::find(uses_only_ins.begin(), uses_only_ins.end(),
                           ins.args[0]) != uses_only_ins.end();
  if (!no_defs)
    regs.erase(regs.begin());
  return regs;
}

std::vector<std::size_t> list_defs(const Instruction &ins) {
  auto regs = list_regs(ins);
  if (regs.empty())
    return {};

  bool no_defs = std::find(uses_only_ins.begin(), uses_only_ins.end(),
                           ins.args[0]) != uses_only_ins.end();
  if (no_defs)
    return {};
  else
    return {regs[0]};
}

Bitmap<std::uint32_t> make_it_bmap(const Instruction &ins,
                                   const std::vector<std::size_t> &idxs) {
  Bitmap<std::uint32_t> res(ins.args.size());
  for (auto p : idxs)
    res.put(p);
  return res;
};

IteratorRange<op_iterator_t>
make_op_range(Instruction &ins, const std::vector<std::size_t> &idxs) {
  auto bmap = make_it_bmap(ins, idxs);
  op_iterator_t beg(ins, 0, bmap);
  op_iterator_t end(ins, ins.args.size(), bmap);
  return IteratorRange<op_iterator_t>(beg, end);
}

IteratorRange<const_op_iterator_t>
make_op_range(const Instruction &ins, const std::vector<std::size_t> &idxs) {
  auto bmap = make_it_bmap(ins, idxs);
  const_op_iterator_t beg(ins, 0, bmap);
  const_op_iterator_t end(ins, ins.args.size(), bmap);
  return IteratorRange<const_op_iterator_t>(beg, end);
}

// op_iterator_t make_op_it(Instruction& ins, const std::vector

} // namespace

bool is_branch(const Instruction &ins) {
  const auto &opname = ins.args[0];
  return std::find(branch_ins.begin(), branch_ins.end(), opname) !=
         branch_ins.end();
}

std::vector<std::string> branch_targets(const Instruction &ins) {
  assert(is_branch(ins));
  return list_labels(ins);
}

void dump(const Instruction &ins, std::ostream &os) {
  gop::Ins(ins.args).dump(os);
}

IteratorRange<op_iterator_t> uses(Instruction &ins) {
  return make_op_range(ins, list_uses(ins));
}

IteratorRange<const_op_iterator_t> uses(const Instruction &ins) {
  return make_op_range(ins, list_uses(ins));
}

IteratorRange<op_iterator_t> defs(Instruction &ins) {
  return make_op_range(ins, list_defs(ins));
}

IteratorRange<const_op_iterator_t> defs(const Instruction &ins) {
  return make_op_range(ins, list_defs(ins));
}

} // namespace isa
