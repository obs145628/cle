#include <iostream>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

#include "lib/thb.hh"

static llvm::LLVMContext g_context;

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: tree-height-balancing <src-ll-file>" << std::endl;
    return 1;
  }

  auto in_file = argv[1];
  llvm::SMDiagnostic err;
  auto mod = llvm::parseIRFile(in_file, err, g_context);
  if (!mod) {
    std::cerr << "failed to load input file\n";
    return 1;
  }

  llvm::errs() << "Loaded module " << mod->getName() << "\n";

  thb_run(*mod);

  mod->print(llvm::errs(), nullptr);

  if (llvm::verifyModule(*mod, &llvm::errs())) {
    llvm::errs() << "Invalid transformations. Failed to pass validation\n";
    return 1;
  }

  return 0;
}
