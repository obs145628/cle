#pragma once

#include "../isa/analysis.hh"
#include "cfg.hh"
#include <logia/md-gfm-doc.hh>

// Divide all blocks in a function into a set of expended basic block
// An EBB is a list of blocks B_1, ..., B_n,
// such that B_1 has multiple preds (or is the entry block)
// and for all b_j, j=2...n, b_j has only one pred, which also belongs to the
// EBB
//
// Each EBB is divided into a list of path, all starting from B_1
// The list of path is sorted by the frequency of this path taken (desc)
//
// The algorithm output is a list of paths
// Each path will be scheduled one by one in that order
// Paths scheduled first are less likely to have code addition to fix code
// motion around blocks
//
// To compute frequency, it assumes that for a branch with N choices, each
// choice can happen with a probability 1/N.

class EbPaths : public isa::FunctionAnalysis {

public:
  using path_t = std::vector<const isa::BasicBlock *>;
  using ebb_t = std::vector<const isa::BasicBlock *>;

  EbPaths(const isa::Function &fun);
  EbPaths(const EbPaths &) = delete;

  const std::vector<const path_t *> &paths() const { return _sorted_paths; }

private:
  const CFG &_cfg;

  std::vector<ebb_t> _ebbs;
  std::vector<std::unique_ptr<path_t>> _all_paths;
  std::map<std::size_t, std::vector<const path_t *>> _ebb_paths;
  std::vector<const path_t *> _sorted_paths;
  std::map<const path_t *, double> _paths_freq;

  std::unique_ptr<logia::MdGfmDoc> _doc;

  void _build();

  void _find_ebbs();
  void _add_ebb(const isa::BasicBlock &head);

  void _find_paths_of_ebb(std::size_t ebb_id);
  void _find_paths_of_ebb_rec(std::size_t ebb_id, const path_t &path);

  void _sort_paths_of_ebb(std::size_t ebb_id);

  double _get_freq(const path_t *path);

  void _dump_paths();
};
