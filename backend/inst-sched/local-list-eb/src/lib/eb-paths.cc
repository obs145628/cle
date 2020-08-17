#include "eb-paths.hh"

#include <algorithm>
#include <logia/program.hh>

EbPaths::EbPaths(const isa::Function &fun)
    : FunctionAnalysis(fun), _cfg(fun.get_analysis<CFG>()) {
  _doc = logia::Program::instance().add_doc<logia::MdGfmDoc>("EBB paths: @" +
                                                             fun.name());
  _build();
}

void EbPaths::_build() {
  _find_ebbs();

  *_doc << "## EBBs Paths List\n";
  for (std::size_t i = 0; i < _ebbs.size(); ++i)
    _find_paths_of_ebb(i);

  for (std::size_t i = 0; i < _ebbs.size(); ++i)
    _sort_paths_of_ebb(i);
  _dump_paths();
}

// Compute list of ebbs
void EbPaths::_find_ebbs() {

  // Find all EBB heads an compute ebb for each one
  std::vector<const isa::BasicBlock *> headers;
  for (auto bb : fun().bbs())
    if (bb == fun().bbs()[0] || _cfg.preds(*bb).size() > 1)
      _add_ebb(*bb);

  *_doc << "## Extended Basic Blocks List\n";
  auto ch = _doc->code();
  for (const auto &ebb : _ebbs) {
    ch << "{";
    for (std::size_t i = 0; i < ebb.size(); ++i) {
      ch << ebb[i]->name();
      if (i + 1 < ebb.size())
        ch << ", ";
    }
    ch << "}\n";
  }
}

void EbPaths::_add_ebb(const isa::BasicBlock &head) {
  std::set<const isa::BasicBlock *> marked;
  std::vector<const isa::BasicBlock *> q;
  q.push_back(&head);
  ebb_t new_ebb;

  while (!q.empty()) {
    auto bb = q.front();
    q.erase(q.begin());
    if (marked.count(bb))
      continue;

    new_ebb.push_back(bb);
    marked.insert(bb);

    for (auto s : _cfg.succs(*bb))
      if (!marked.count(s) && _cfg.preds(*s).size() < 2)
        q.push_back(s);
  }

  _ebbs.push_back(new_ebb);
}

void EbPaths::_find_paths_of_ebb(std::size_t ebb_id) {
  const auto &ebb = _ebbs[ebb_id];
  _find_paths_of_ebb_rec(ebb_id, {ebb.front()});

  auto ch = _doc->code();
  ch << "EBB  {";
  for (std::size_t i = 0; i < ebb.size(); ++i) {
    ch << ebb[i]->name();
    if (i + 1 < ebb.size())
      ch << ", ";
  }
  ch << "}\n";

  const auto &paths = _ebb_paths.at(ebb_id);
  for (std::size_t i = 0; i < paths.size(); ++i) {
    const auto &path = *paths[i];
    ch << "Path #" << (i + 1) << ": {";
    for (std::size_t i = 0; i < path.size(); ++i) {
      ch << path[i]->name();
      if (i + 1 < path.size())
        ch << ", ";
    }
    ch << "}\n";
  }
}

void EbPaths::_find_paths_of_ebb_rec(std::size_t ebb_id, const path_t &path) {
  const auto &ebb = _ebbs[ebb_id];
  auto last = path.back();
  bool is_end = true;
  for (auto s : _cfg.succs(*last)) {
    if (std::find(ebb.begin(), ebb.end(), s) != ebb.end() &&
        std::find(path.begin(), path.end(), s) == path.end()) {
      // Only add blocks in ebb, not visited in current path yet
      is_end = false;
      auto next_path = path;
      next_path.push_back(s);
      _find_paths_of_ebb_rec(ebb_id, next_path);
    }
  }

  if (is_end) {
    _all_paths.push_back(std::make_unique<path_t>(path));
    _ebb_paths[ebb_id].push_back(_all_paths.back().get());
  }
}

void EbPaths::_sort_paths_of_ebb(std::size_t ebb_id) {
  auto paths = _ebb_paths.at(ebb_id);

  std::sort(paths.begin(), paths.end(), [this](auto lhs, auto rhs) {
    return _get_freq(lhs) > _get_freq(rhs);
  });

  _sorted_paths.insert(_sorted_paths.end(), paths.begin(), paths.end());
}

double EbPaths::_get_freq(const path_t *path) {
  auto it = _paths_freq.find(path);
  if (it != _paths_freq.end())
    return it->second;

  double freq = 1.;
  for (std::size_t i = 0; i + 1 < path->size(); ++i) {
    auto bb = (*path)[i];
    freq *= 1. / _cfg.succs(*bb).size();
  }

  _paths_freq.emplace(path, freq);
  return freq;
}

void EbPaths::_dump_paths() {
  *_doc << "## Complete Paths List\n";
  auto ch = _doc->code();

  for (auto path : _sorted_paths) {
    ch << "{";
    for (std::size_t i = 0; i < path->size(); ++i) {
      ch << (*path)[i]->name();
      if (i + 1 < path->size())
        ch << ", ";
    }
    ch << "}\n";
  }
}
