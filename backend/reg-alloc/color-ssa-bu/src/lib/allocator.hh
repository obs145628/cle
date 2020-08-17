#pragma once

#include "../isa/module.hh"
#include "../utils/dyn-graph.hh"
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
//
// Before coloring, live range coaslescing is applied: if there is a mov from a
// to b, and no interference between a and b, the mov is deleted, and all refs
// to b are replaced by refs to a
//
// Coloring scheme is 2-steps:
// 1) takes unconstrained live ranges in inference graph and store them
// in a stack. Each time once is taken, it's removed with its edges.
// If there is only constrained regs left, a spill metric is used, and the one
// with lower cost is used.
//
// 2) Take live-ranges from stack (reverse order, and restore the vertex and its
// edges to the interference graph), and try to assign it a color.
// If fail, it gets spilled and everything is restarted
//
// Algorithm Register Allocation: Bottom-Up Coloring - Engineer a Compiler p704
// Algorithm Register Allocation: Coaslescing Copies - Engineer a Compiler p706

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
  std::size_t _choose_lr_alloc(const std::vector<std::size_t> &lrs);
  std::size_t _try_assign(std::size_t lr, const DynGraph<std::size_t> &ig);
  void _spill(std::size_t lr);
  std::size_t _get_first_spill_pos();

  void _rewrite_code();

  void _dump_assign();
};
