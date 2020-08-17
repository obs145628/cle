#pragma once

#include "gop.hh"
#include "module.hh"

struct CallInfos {
  std::string src;
  std::string dst;
  int val;
};

// Build a IModule given classic module
// Divide module into functions and basic blocks
// Check if whole module is well formed
// Parse and store call_freqs in cc_freqs (comment after call instruction)
std::unique_ptr<Module> load_module(const gop::Module &mod,
                                    std::vector<CallInfos> &cc_freqs);
std::unique_ptr<Module> load_module(std::istream &i,
                                    std::vector<CallInfos> &cc_freqs);
std::unique_ptr<Module> load_module(const std::string &path,
                                    std::vector<CallInfos> &cc_freqs);

// Convert a module to a gop::Module, with all functions ans ins
// Labels are basic blocks labels
gop::Module mod2gop(const Module &mod);
