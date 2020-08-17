#pragma once

#include "module.hh"
#include <gop10/module.hh>

// Build a Module given gop module
// Divide module into functions and basic blocks
// Check if whole module is well formed
std::unique_ptr<Module> load_module(const gop::Module &mod);
std::unique_ptr<Module> load_module(std::istream &is);
std::unique_ptr<Module> load_module(const std::string &path);

// Convert a module to a gop::Module
// Labels are basic blocks labels
gop::Module mod2gop(const Module &mod);
