#include "context.hh"

#include <utils/cli/err.hh>
#include <utils/str/format-string.hh>

Context::Context() {}

std::unique_ptr<Context> Context::make(const std::string &arch_str) {
  const auto &regs = _get_builders();
  auto it = regs.find(arch_str);
  PANIC_IF(it == regs.end(), FMT_OSS("Uknown arch `" << arch_str << "'"));
  return std::unique_ptr<Context>(it->second());
}

void Context::init() { _on_init(); }

void Context::preprocess_ir(Module &mod) {
  for (auto &p : _passes)
    p->preprocess(mod);
}

void Context::update_asm(Module &mod) {
  for (auto &p : _passes)
    p->update_asm(mod);
}

isa::Context &Context::ir_ctx() {
  if (!_ir_ctx)
    _ir_ctx = std::make_unique<isa::Context>(_get_isa_ir_file_path());
  return *_ir_ctx;
}

isa::Context &Context::asm_ctx() {
  if (!_asm_ctx)
    _asm_ctx = std::make_unique<isa::Context>(_get_isa_asm_file_path());
  return *_asm_ctx;
}

Rules &Context::rules() {
  if (!_rules)
    _rules = std::make_unique<Rules>(ir_ctx(), _get_codegen_rules_file_path());
  return *_rules;
}

std::map<std::string, std::function<Context *()>> &Context::_get_builders() {
  static std::map<std::string, std::function<Context *()>> builders;
  return builders;
}
