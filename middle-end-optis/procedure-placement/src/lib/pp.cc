#include "pp.hh"

#include <algorithm>
#include <iostream>

namespace {

struct CGVal {
  std::size_t src;
  std::size_t dst;
  int val;

  CGVal(std::size_t src, std::size_t dst, int val)
      : src(src), dst(dst), val(val) {}
};

class CGQueue {
public:
  void add(const CGVal &ci) {
    if (ci.src == ci.dst)
      return; // ignore self-loop

    CGVal *act = nullptr; // find already available src -> dst edge
    for (auto &item : _q)
      if (item.src == ci.src && item.dst == ci.dst) {
        act = &item;
        break;
      }

    if (act)
      act->val += ci.val;
    else {

      auto it = std::upper_bound(
          _q.begin(), _q.end(), ci,
          [](const CGVal &a, const CGVal &b) { return a.val < b.val; });

      _q.insert(it, ci);
    }
  }

  CGVal pop() {
    CGVal res = _q.back();
    _q.pop_back();
    return res;
  }

  void rename(std::size_t prev, std::size_t next) {
    for (auto &item : _q) {
      if (item.src == prev)
        item.src = next;
      if (item.dst == prev)
        item.dst = next;
    }

    // Rebuild queue to handle doublons that may appear because of renaming: sum
    // values
    auto vals = std::move(_q);
    for (auto &item : vals)
      add(item);
  }

  bool empty() const { return _q.empty(); }

  void dump(std::ostream &os) {
    os << "CGQ: {";

    for (auto it = _q.rbegin(); it != _q.rend(); ++it)
      os << "(P" << it->src << ", P" << it->dst << ", " << it->val << "); ";

    os << "}\n";
  }

private:
  std::vector<CGVal> _q;
};

using fun_list_t = std::vector<std::string>;

class PP {
public:
  PP(Module &mod) : _mod(mod) {}

  void run(const std::vector<CallInfos> &cg_freqs) {
    // Step 1: Initialize queue and list
    std::map<std::string, std::size_t> n2i;
    for (const auto &fun : _mod.fun()) {
      _lists.push_back({fun.name()});
      n2i.emplace(fun.name(), n2i.size());
    }
    for (const auto &ci : cg_freqs)
      _cgq.add(CGVal(n2i[ci.src], n2i[ci.dst], ci.val));

    _dump(std::cout);

    // Step 2: Reduce graph until queue empty
    while (!_cgq.empty()) {
      _reduce();
      _dump(std::cout);
    }

    // At this step, there is M lists, one for each connected component of the
    // callgraph The order in each list is chosen to increase cache locality
    // Procedures should be put by following the order inside the lists
    // The position from one list to another doesn't matter
    std::vector<Function *> new_order;
    for (const auto &l : _lists)
      for (const auto &f : l)
        new_order.push_back(_mod.get_fun(f));
    _mod.fun_list().reorder(new_order);
  }

private:
  Module &_mod;
  CGQueue _cgq;
  std::vector<fun_list_t> _lists;

  // Reduce the graph by poping from the queue (most freq edge)
  // Then combine the 2 lists related to the vertices of the edge in one
  // Need to update the values in the queue (rename y by x)
  void _reduce() {
    auto next = _cgq.pop();
    fun_list_t &lx = _lists[next.src];
    fun_list_t &ly = _lists[next.dst];
    assert(!lx.empty());
    assert(!ly.empty());

    _cgq.rename(next.dst, next.src);
    lx.insert(lx.end(), ly.begin(), ly.end());
    ly.clear();
  }

  void _dump(std::ostream &os) {
    for (std::size_t i = 0; i < _lists.size(); ++i) {
      if (_lists[i].empty())
        continue;
      os << "P" << i << ": {";
      for (auto &f : _lists[i])
        os << f << ' ';
      os << "}; ";
    }
    os << "\n";
    _cgq.dump(os);
    os << "\n";
  }
};
} // namespace

void pp_run(Module &mod, const std::vector<CallInfos> &cg_freqs) {
  PP pp(mod);
  pp.run(cg_freqs);
}
