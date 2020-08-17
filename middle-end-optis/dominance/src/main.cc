#include <cstdlib>
#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

#include "lib/dom.hh"

static llvm::LLVMContext g_context;

std::unique_ptr<llvm::Module> load_module(const char *path) {
  llvm::SMDiagnostic err;
  auto mod = llvm::parseIRFile(path, err, g_context);
  if (!mod) {
    llvm::errs() << "failed to load input file\n";
    std::exit(1);
  }

  if (llvm::verifyModule(*mod, &llvm::errs())) {
    llvm::errs() << "Invalid module\n";
    std::exit(1);
  }

  llvm::errs() << "Loaded module " << mod->getName() << "\n";
  return mod;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: dominance <src-ll-file> " << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  auto mod = load_module(in_file);

  dom_run(*mod);

  mod->print(llvm::errs(), nullptr);

  return 0;
}
