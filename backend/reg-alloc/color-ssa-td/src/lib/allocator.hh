#pragma once

#include "../isa/module.hh"
#include "../utils/graph.hh"
#include "block-freq.hh"
#include <logia/md-gfm-doc.hh>

// Graph Coloring on SSA form, Top-Down approach
// Perform register allocation for a whole function
// Take input function as SSA form.
// Compute live-ranges and remove SSA form
// Compute spill costs and inference cost of every live range
// Try to assign registers to every live range using coloring algorithm
// If coloring fails, spill failed live-range, and rebuilt all structures to try
// again coloring
// Coloring scheme first divide live-ranges into constrained ones, sorted by
// spill cost, and constrained ones.
// Try to assign constrained ones first, then do unconstrained ones (cannot
// fail)
//
// Algorithm Register Allocation: Top-Down Coloring - Engineer a Compiler p702

// Hardware registers: h0 -> h (hr_count - 1) + hsp
// Arguments stored in sp[4], sp[8], ...
// Return register: ignored
// Stack register sp: always mapped to hsp
//
//

class Allocator {

public:
  Allocator(isa::Function &fun, std::size_t hr_count);

  static void apply(isa::Module &mod, std::size_t hr_count);

  void run();

private:
  isa::Function &_fun;
  const isa::Context &_ctx;
  std::size_t _hr_count;

  std::unique_ptr<isa::Module> _if_mod;
  isa::Function *_if_fun;

  std::size_t _next_spill_pos;
  std::vector<std::size_t> _assignments;

  std::unique_ptr<logia::MdGfmDoc> _doc;

  void _compute_live_ranges();

  bool _step_color();
  std::size_t _try_assign(std::size_t lr);
  void _spill(std::size_t lr);
  std::size_t _get_first_spill_pos();

  void _rewrite_code();

  void _dump_assign();
};
