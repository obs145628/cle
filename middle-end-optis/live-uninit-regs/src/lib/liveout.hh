#pragma once

#include <map>
#include <set>

#include "imodule.hh"

using regs_set_t = std::set<std::string>;

using liveout_res_t = std::map<const BasicBlock *, regs_set_t>;

// Compute the sets Liveout(bb) for every bb in mod
// Liveout(bb) is the set of all registers live at the exit (right after the
// last instruction) of bb
// A register v is live at point p if and only if it exists a path in the CFG
// from p to a use of v, along which v is not redefined
//
// Algorithm LiveOut Variables - Engineer a Compiler p445
liveout_res_t run_liveout(const IModule &mod);
