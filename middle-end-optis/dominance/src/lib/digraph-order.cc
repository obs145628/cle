#include "digraph-order.hh"

#include <cassert>
#include <vector>

namespace {

enum class GOrder {
  PRE,
  POST,
  REV_POST,
};

class DFS {
public:
  DFS(const Digraph &g, GOrder order) : _g(g), _order(order) {}

  std::vector<std::size_t> run() {
    _marked.assign(_g.v(), 0);
    for (std::size_t u = 0; u < _g.v(); ++u)
      if (!_marked[u])
        _dfs(u);

    assert(_res.size() == _g.v());

    if (_order == GOrder::REV_POST)
      _res = std::vector<std::size_t>{_res.rbegin(), _res.rend()};
    return _res;
  }

private:
  const Digraph &_g;
  GOrder _order;
  std::vector<int> _marked;
  std::vector<std::size_t> _res;

  void _dfs(std::size_t u) {
    _marked[u] = 1;

    if (_order == GOrder::PRE)
      _res.push_back(u);

    for (auto v : _g.succs(u))
      if (!_marked[v])
        _dfs(v);

    if (_order == GOrder::POST || _order == GOrder::REV_POST)
      _res.push_back(u);
  }
};

} // namespace

std::vector<std::size_t> preorder(const Digraph &g) {
  return (DFS{g, GOrder::PRE}).run();
}

std::vector<std::size_t> postorder(const Digraph &g) {
  return (DFS{g, GOrder::POST}).run();
}

std::vector<std::size_t> rev_postorder(const Digraph &g) {
  return (DFS{g, GOrder::REV_POST}).run();
}
