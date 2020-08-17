#include "allocator.hh"

#include <algorithm>
#include <cassert>

#include <logia/program.hh>

namespace {

bool is_reg(const std::string &str) { return str.front() == '%'; }

// Numbers of extra registers needed if spiiling
// Only one is needed to load a spilled value
// But if all operands (max 2) are spilled, then 2 will be needed
constexpr std::size_t F_REGS = 2;

} // namespace

Allocator::Allocator(isa::BasicBlock &bb, std::size_t hr_count)
    : _bb(bb), _ctx(_bb.parent().parent().ctx()), _hr_count(hr_count) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>(
      "Allocator for @" + bb.parent().name() + ":@" + bb.name());
  _init();
}

void Allocator::apply(isa::Module &mod, std::size_t hr_count) {
  for (auto fun : mod.funs()) {
    assert(fun->bbs().size() == 1);
    Allocator alloc(*fun->bbs().front(), hr_count);
    alloc.run();
  }
}

void Allocator::run() {
  // Make a first pass to compute the frequency of each reg
  _compute_freq();

  // Assign hardware registers
  _choose_hard_regs();

  // Assign spilled registers
  _choose_spills();

  // Rewrite code
  _rewrite();
}

void Allocator::_init() {
  _special["arg0"] = "h0";
  _special["arg1"] = "h1";
  _special["arg2"] = "h2";
  _special["arg3"] = "h3";
  _special["ret"] = "h0";
  _special["sp"] = "hsp";
}

// Go through all ins, and count how many time each reg is used
// Also count special regs, this is only to know if they are used
// This pass is also used to figure out where the spill address should start
// (code might already use the stack, spill must be inserted after)
void Allocator::_compute_freq() {
  int max_stack_off = 0;

  for (const auto &ins : _bb.code()) {
    for (const auto &arg : ins)
      if (is_reg(arg)) {
        _freq.emplace(arg.substr(1), 0);
        ++_freq[arg.substr(1)];
      }

    if ((ins[0] == "load" || ins[0] == "store") && ins[2] == "%sp")
      max_stack_off = std::max(max_stack_off, std::atoi(ins[3].c_str()));
  }

  _spill_start_addr = max_stack_off + 4;

  *_doc << "## Frequencies\n";
  auto ch = _doc->code();
  for (auto f : _freq)
    ch << f.first << ": " << f.second << "\n";
}

// Choose the mapping from virtual reg to hardware regs
// Only alloc more used once, others will be spilled
void Allocator::_choose_hard_regs() {
  // Compute reg properties
  if (_freq.count("arg3"))
    _args_count = 4;
  else if (_freq.count("arg2"))
    _args_count = 3;
  else if (_freq.count("arg1"))
    _args_count = 2;
  else if (_freq.count("arg0"))
    _args_count = 1;
  else
    _args_count = 0;

  _virt_count = 0;
  for (auto f : _freq)
    if (!_special.count(f.first))
      ++_virt_count;

  bool need_spill = _args_count + _virt_count > _hr_count;
  std::size_t alloc_count =
      need_spill ? _hr_count - _args_count - F_REGS : _virt_count;
  _spill_count = _virt_count - alloc_count;

  // Sort virtual regs by frequency
  for (auto f : _freq)
    if (!_special.count(f.first))
      _sorted.push_back(f.first);

  std::sort(_sorted.begin(), _sorted.end(),
            [this](const auto &a, const auto &b) {
              return _freq.at(a) > _freq.at(b);
            });

  // Assign registers
  std::size_t next_reg = _args_count;
  for (std::size_t i = 0; i < alloc_count; ++i) {
    auto reg_id = next_reg++;
    assert(reg_id < _hr_count);
    _mapped[_sorted[i]] = "h" + std::to_string(reg_id);
  }

  *_doc << "## Stats\n";
  *_doc << "- args count: " << _args_count << "\n";
  *_doc << "- virt count: " << _virt_count << "\n";
  *_doc << "- hard count: " << _hr_count << "\n";
  *_doc << "- alloc count: " << alloc_count << "\n";
  *_doc << "- spill count: " << _spill_count << "\n";
  *_doc << "- spill start offset: " << _spill_start_addr << "\n";

  *_doc << "## Assigned registers\n";
  for (auto it : _mapped)
    *_doc << "- " << it.first << ": " << it.second << "\n";
}

// Choose the mapping from virtual regs to sp offset for regs that need to be
// spilled
// Also apply for special case ret if arg0 is used
void Allocator::_choose_spills() {
  bool spill_ret = _freq.count("ret") && _freq.count("arg0");
  if (!_spill_count && !spill_ret)
    return;

  std::size_t next_spill = _spill_start_addr;

  if (spill_ret) {
    _spilled["ret"] = next_spill;
    next_spill += 4;
  }

  std::size_t alloc_count = _virt_count - _spill_count;

  for (std::size_t i = 0; i < _spill_count; ++i) {
    const auto &reg = _sorted[alloc_count + i];
    _spilled[reg] = next_spill;
    next_spill += 4;
  }

  *_doc << "## Spilled registers\n";
  for (auto it : _spilled)
    *_doc << "- " << it.first << ": " << it.second << "\n";
}

// Return a temporary register used to spill a value into the stack
std::string Allocator::_get_tmp_spill() {
  auto id = _spill_next_tmp++;
  assert(id < F_REGS);
  return "h" + std::to_string(_hr_count - 1 - id);
}

void Allocator::_rewrite() {
  std::vector<std::vector<std::string>> new_code;

  for (const auto &ins : _bb.code()) {
    auto ins_code = _rewrite_ins(ins);
    new_code.insert(new_code.end(), ins_code.begin(), ins_code.end());
  }

  _bb.code() = new_code;

  *_doc << "## Rewriten code\n";
  _bb.dump_code(*_doc);
}

// Rewrite code for an instruction
// Change all registers to hardware one
// Add extra ins for spilling if necessary
std::vector<std::vector<std::string>>
Allocator::_rewrite_ins(const std::vector<std::string> &ins) {
  _code_pre.clear();
  _code_post.clear();
  std::vector<std::vector<std::string>> res;
  _spill_next_tmp = 0;

  // Special case for ret instruction
  // If ret reg is spilled, need to load the value in r0 first
  if (ins[0] == "ret" && _spilled.count("ret"))
    _code_pre.push_back({std::string("load"), std::string("%h0"),
                         std::string("%hsp"),
                         std::to_string(_spilled.at("ret"))});

  isa::Ins cins(_ctx, ins);
  std::vector<std::string> new_ins = ins;

  // rewrite uses first
  for (std::size_t i = 1; i < ins.size(); ++i)
    if (cins.get_arg_kind(i - 1) == isa::ARG_KIND_USE)
      new_ins[i] = "%" + _rewrite_use(ins[i].substr(1));

  _spill_next_tmp = 0;
  // then rewrite defs
  for (std::size_t i = 1; i < ins.size(); ++i)
    if (cins.get_arg_kind(i - 1) == isa::ARG_KIND_DEF)
      new_ins[i] = "%" + _rewrite_def(ins[i].substr(1));

  res.insert(res.end(), _code_pre.begin(), _code_pre.end());
  res.push_back(new_ins);
  res.insert(res.end(), _code_post.begin(), _code_post.end());
  return res;
}

std::string Allocator::_rewrite_use(const std::string &arg) {
  if (_mapped.count(arg))
    return _mapped.at(arg);
  else if (_spilled.count(arg)) {
    auto tmp = _get_tmp_spill();
    _code_pre.push_back({std::string("load"), "%" + tmp, std::string("%hsp"),
                         std::to_string(_spilled.at(arg))});
    return tmp;
  }

  else if (_special.count(arg))
    return _special.at(arg);

  else
    assert(0);
}

std::string Allocator::_rewrite_def(const std::string &arg) {
  if (_mapped.count(arg))
    return _mapped.at(arg);
  else if (_spilled.count(arg)) {
    auto tmp = _get_tmp_spill();
    _code_post.push_back({std::string("store"), "%" + tmp, std::string("%hsp"),
                          std::to_string(_spilled.at(arg))});
    return tmp;
  }

  else if (_special.count(arg))
    return _special.at(arg);

  else
    assert(0);
}
