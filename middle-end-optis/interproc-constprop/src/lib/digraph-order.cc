#include "digraph-order.hh"

#include <cassert>
#include <vector>

namespace {

class DFS {
public:
  DFS(const Digraph &g, DFSOrder order, std::size_t start,
      bool visit_unreachable)
      : _g(g), _order(order), _start(start),
        _visit_unreachable(visit_unreachable) {}

  std::vector<std::size_t> run() {
    _marked.assign(_g.v(), 0);

    _dfs(_start);

    if (_visit_unreachable)
      for (std::size_t u = 0; u < _g.v(); ++u)
        if (!_marked[u])
          _dfs(u);

    assert(!_visit_unreachable || _res.size() == _g.v());

    if (_order == DFSOrder::REV_POST)
      _res = std::vector<std::size_t>{_res.rbegin(), _res.rend()};
    return _res;
  }

private:
  const Digraph &_g;
  DFSOrder _order;
  std::size_t _start;
  bool _visit_unreachable;
  std::vector<int> _marked;
  std::vector<std::size_t> _res;

  void _dfs(std::size_t u) {
    _marked[u] = 1;

    if (_order == DFSOrder::PRE)
      _res.push_back(u);

    for (auto v : _g.succs(u))
      if (!_marked[v])
        _dfs(v);

    if (_order == DFSOrder::POST || _order == DFSOrder::REV_POST)
      _res.push_back(u);
  }
};

} // namespace

std::vector<std::size_t> digraph_dfs(const Digraph &g, DFSOrder order,
                                     std::size_t start,
                                     bool visit_unreachable) {
  return (DFS{g, order, start, visit_unreachable}).run();
}
