#include <cstdlib>
#include <cstring>
#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>
#include <vector>

#include "lib/dce.hh"
#include "lib/dcfe.hh"
#include "lib/unreachable.hh"

static llvm::LLVMContext g_context;

void check_mod(const llvm::Module &mod) {
  if (llvm::verifyModule(mod, &llvm::errs())) {
    llvm::errs() << "Invalid module\n";
    std::exit(1);
  }
}

std::unique_ptr<llvm::Module> load_module(const char *path) {
  llvm::SMDiagnostic err;
  auto mod = llvm::parseIRFile(path, err, g_context);
  if (!mod) {
    llvm::errs() << "failed to load input file\n";
    std::exit(1);
  }

  check_mod(*mod);
  llvm::errs() << "Loaded module " << mod->getName() << "\n";
  return mod;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: dead-code-elim <src-ll-file> " << std::endl;
    return 1;
  }
  bool run_dce = false;
  bool run_dcfe = false;
  bool run_unreachable = false;
  for (int i = 0; i < argc; ++i) {
    if (!std::strcmp(argv[i], "--dce"))
      run_dce = true;
    if (!std::strcmp(argv[i], "--dcfe"))
      run_dcfe = true;
    if (!std::strcmp(argv[i], "--ue"))
      run_unreachable = true;
  }

  auto in_file = argv[1];
  auto mod = load_module(in_file);

  if (run_dce)
    dce_run(*mod);
  if (run_dcfe)
    dcfe_run(*mod);
  if (run_unreachable)
    unreachable_run(*mod);

  mod->print(llvm::errs(), nullptr);
  check_mod(*mod);

  return 0;
}
