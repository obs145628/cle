
#pragma once

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>

#include "../isa/isa.hh"
#include "module-pass.hh"
#include "rules.hh"

class Context {

public:
  Context(const Context &) = delete;
  virtual ~Context() = default;
  Context &operator=(const Context &) = delete;

  static std::unique_ptr<Context> make(const std::string &arch_str);

  template <class T> static void register_class(const std::string &name) {
    std::function<Context *()> cons = []() { return new T; };
    _get_builders().emplace(name, cons);
  }

  void init();

  // Preprocess input IR
  void preprocess_ir(Module &mod);

  // Update ouput ASM
  void update_asm(Module &mod);

  isa::Context &ir_ctx();
  isa::Context &asm_ctx();
  Rules &rules();

  template <class T> T &add_module_pass();

  template <class T> T &get_module_pass();

private:
  std::unique_ptr<isa::Context> _ir_ctx;
  std::unique_ptr<isa::Context> _asm_ctx;
  std::unique_ptr<Rules> _rules;
  std::vector<std::unique_ptr<ModulePass>> _passes;
  std::map<std::type_index, ModulePass *> _passes_map;

protected:
  virtual void _on_init() = 0;
  virtual std::string _get_isa_ir_file_path() = 0;
  virtual std::string _get_isa_asm_file_path() = 0;
  virtual std::string _get_codegen_rules_file_path() = 0;

  static std::map<std::string, std::function<Context *()>> &_get_builders();

  Context();
};

template <class T> T &Context::add_module_pass() {
  auto pass = std::make_unique<T>();
  auto &res = *pass;
  _passes.push_back(std::move(pass));
  assert(_passes_map.emplace(typeid(T), &res).second);
  return res;
}

template <class T> T &Context::get_module_pass() {
  return dynamic_cast<T &>(*_passes_map.at(typeid(T)));
}

#define REGISTER_CONTEXT(Name, ClassName)                                      \
  namespace {                                                                  \
  struct __Context_Register_Helper {                                           \
    __Context_Register_Helper() {                                              \
      ::Context::register_class<ClassName>(Name);                              \
    }                                                                          \
  };                                                                           \
  __Context_Register_Helper __global_context_register_helper{};                \
  }
