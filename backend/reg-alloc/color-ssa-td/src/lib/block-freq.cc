#include "block-freq.hh"

#include <logia/program.hh>

namespace {

constexpr std::size_t LOOP_COUNT = 10;

}

BlockFreq::BlockFreq(const isa::Function &fun)
    : isa::FunctionAnalysis(fun), _cfg(fun.get_analysis<CFG>()) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>("BB Freq: @" +
                                                             fun.name());
  _run();
  _doc = nullptr;
}

void BlockFreq::_run() {
  // Find back edges first (needed to detect loops)
  _find_back_edges();

  // Start with a freq of one on entry block and visit all blocks with DFS
  double freq = 1;
  const auto &entry = *fun().bbs().front();
  _eval(entry, freq);

  *_doc << "## Frequencies\n";
  for (auto it : _freqs)
    *_doc << "- `@" << it.first->name() << ": " << it.second << "`\n";
}

void BlockFreq::_find_back_edges() {
  std::set<const isa::BasicBlock *> visited;
  std::set<const isa::BasicBlock *> on_stack;
  _find_back_edges_rec(*fun().bbs()[0], visited, on_stack);

  if (_back_edges.empty())
    return;
  *_doc << " ## Back edges\n";
  for (auto be : _back_edges)
    *_doc << "- `@" << be.first->name() << " -> @" << be.second->name()
          << "`\n";
}

void BlockFreq::_find_back_edges_rec(
    const isa::BasicBlock &bb, std::set<const isa::BasicBlock *> &visited,
    std::set<const isa::BasicBlock *> &on_stack) {
  visited.insert(&bb);
  on_stack.insert(&bb);

  for (auto next : _cfg.succs(bb)) {
    if (on_stack.count(next)) {
      // Found cycle
      _back_edges.emplace_back(&bb, next);
    } else if (!visited.count(next))
      _find_back_edges_rec(*next, visited, on_stack);
  }

  on_stack.erase(&bb);
}

std::set<const isa::BasicBlock *>
BlockFreq::_back_edge_preds(const isa::BasicBlock &bb) {
  std::set<const isa::BasicBlock *> res;
  for (auto be : _back_edges)
    if (be.second == &bb)
      res.insert(be.first);
  return res;
}

std::set<const isa::BasicBlock *>
BlockFreq::_back_edge_succs(const isa::BasicBlock &bb) {
  std::set<const isa::BasicBlock *> res;
  for (auto be : _back_edges)
    if (be.first == &bb)
      res.insert(be.second);
  return res;
}

void BlockFreq::_eval(const isa::BasicBlock &bb, double freq) {
  if (!_back_edge_preds(bb).empty()) {
    // bb is a loop entry
    // multiply freq by an estimation of the number of loop iterations
    freq *= LOOP_COUNT;
  }

  // Add freq to current block freq (bb may have multiple preds)
  if (!_freqs.count(&bb))
    _freqs[&bb] = 0;
  _freqs[&bb] += freq;

  auto succs = _cfg.succs(bb);
  if (succs.empty())
    return;

  // Freq of visit succ same for all
  auto be_succs = _back_edge_succs(bb);
  auto next_freq = freq / succs.size();
  for (auto s : succs)
    if (!be_succs.count(s)) // Don't follow a back edge
      _eval(*s, next_freq);
}
