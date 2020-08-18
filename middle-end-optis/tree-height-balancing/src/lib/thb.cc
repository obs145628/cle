#include "thb.hh"

#include <cstring>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <queue>
#include <set>
#include <utility>

namespace {

using pq_item_t = std::pair<llvm::Value *, unsigned>;

struct PqLessThan {
  bool operator()(const pq_item_t &a, const pq_item_t &b) {
    return a.second > b.second;
  }
};

using pq_t = std::priority_queue<pq_item_t, std::vector<pq_item_t>, PqLessThan>;

void dump_pq(const pq_t &pq, const char *name) {

  pq_t cpy = pq;

  llvm::errs() << "queue " << name << " {\n";
  while (!cpy.empty()) {
    auto &it = cpy.top();
    llvm::errs() << "  " << *it.first << "   (" << it.second << ")\n";
    cpy.pop();
  }
  llvm::errs() << "}\n";
}

class THB {
public:
  THB(llvm::BasicBlock &bb) : _bb(bb) {}

  void run() {
    auto roots = _find_roots();
    dump_pq(roots, "roots");

    while (!roots.empty()) {
      auto root = roots.top().first;
      roots.pop();
      _balance(root);
    }

    // Replace and remove all roots
    for (auto it : _new_roots)
      it.first->replaceAllUsesWith(it.second);
    _cleanup_roots(_all_roots);
  }

private:
  // Basic block being processed
  llvm::BasicBlock &_bb;

  // All roots find by the algorithms
  std::set<llvm::Value *> _all_roots;

  // Ranks assigned to every built root
  std::map<llvm::Value *, unsigned> _ranks;

  // Map from old to new roots
  // Code Replace can only be donne after all roots have been handled
  std::map<llvm::Value *, llvm::Value *> _new_roots;

  // Find all roots: nodes at the root of a computation tree that can be
  // balanced The root is the only node of the tree that cannot be changed
  // (value used later)
  // Also store root in _all_roots uset
  //
  // I am not really sure why roots are sorted by operation priority
  pq_t _find_roots() {
    pq_t res;

    for (auto &ins : _bb) {

      // can only be applied to commutative and associative operators
      if (ins.getOpcode() == llvm::Instruction::Add ||
          ins.getOpcode() == llvm::Instruction::Mul) {
        int op_prio = ins.getOpcode() == llvm::Instruction::Add ? 1 : 2;
        // no use means unused computation anyway, can just skip it
        if (ins.getNumUses() == 0)
          continue;

        // A node is a root if:
        // - it has more than one use
        // - or its only use is for a different operator
        bool is_root = ins.getNumUses() > 1 ||
                       ins.user_back()->getOpcode() != ins.getOpcode();

        // Order nodes by priority
        if (is_root) {
          res.emplace(&ins, op_prio);
          _all_roots.insert(&ins);
        }
      }
    }

    return res;
  }

  unsigned _balance(llvm::Value *root) {
    // Check if root already built
    auto it = _ranks.find(root);
    if (it != _ranks.end())
      return it->second;

    pq_t q;
    _flatten(q, root, root);
    llvm::errs() << "flatten " << *root;
    dump_pq(q, "");
    auto iroot = llvm::dyn_cast<llvm::Instruction>(root);
    unsigned rank = _rebuild(q, iroot);
    _ranks.emplace(root, rank);
    return rank;
  }

  // Flatten a tree of operations into linear operations, with their associated
  // rank
  // `root` is used to make the difference between the real root of the tree to
  // be flatenned, and other roots while recursing, that need to be rebuild
  // first.
  void _flatten(pq_t &pq, llvm::Value *val, llvm::Value *root) {

    // Constants are 0-rank to apply constant folding
    if (llvm::isa<llvm::ConstantInt>(val)) {
      pq.emplace(val, 0);
      return;
    }

    // All extern vars have rank 1 (cannot rebuild their tree)
    if (_is_uevar(val)) {
      pq.emplace(val, 1);
      return;
    }

    // Root must always be built
    if (val != root && _all_roots.find(val) != _all_roots.end()) {
      unsigned rank = _balance(val);
      pq.emplace(val, rank);
      return;
    }

    // Flatten all operands
    auto ins = llvm::dyn_cast<llvm::Instruction>(val);
    for (std::size_t i = 0; i < ins->getNumOperands(); ++i)
      _flatten(pq, ins->getOperand(i), root);
  }

  // Rebuild `root` into a more balanced computation tree
  // q contains all indivisible elements of the computation, with associated
  // rank.
  // root = q[0] op q[1] op q[2] op ... with op commutative associative,
  // the op of root.
  // Rebuild a new sequence of instructions to compute root,
  // using a balanced computation tree
  // insert new value in _new_roots
  unsigned _rebuild(pq_t &q, llvm::Instruction *root) {
    unsigned op = root->getOpcode();

    llvm::IRBuilder<> builder(root);

    while (true) {
      assert(q.size() >= 2);
      auto left = q.top();
      q.pop();
      auto right = q.top();
      q.pop();

      // Constant folding
      // Actually this isn't needed because builder already does constant
      // folding when all operands of CreateXXX are consts
      if (llvm::isa<llvm::ConstantInt>(left.first) &&
          llvm::isa<llvm::ConstantInt>(right.first)) {
        auto lconst =
            llvm::dyn_cast<llvm::ConstantInt>(left.first)->getZExtValue();
        auto rconst =
            llvm::dyn_cast<llvm::ConstantInt>(right.first)->getZExtValue();
        auto res =
            op == llvm::Instruction::Add ? lconst + rconst : lconst * rconst;

        llvm::Value *val = llvm::ConstantInt::get(left.first->getType(), res);
        q.emplace(val, 0);
        continue;
      }

      llvm::Value *val = op == llvm::Instruction::Add
                             ? builder.CreateAdd(left.first, right.first)
                             : builder.CreateMul(left.first, right.first);
      unsigned val_rank = left.second + right.second;

      // Empty queue, replace root
      if (q.empty()) {
        _new_roots.emplace(root, val);
        return val_rank;
      }

      // Push intermediate result to queue and keep building
      q.emplace(val, val_rank);
    }
  }

  // Upward Exposed variable
  // val defined in another basic block than b
  //
  // for this example, a uevar is also any value that cannot be part of a op
  // tree so any op that is not a add/sub
  bool _is_uevar(llvm::Value *val) {
    auto ins = llvm::dyn_cast<llvm::Instruction>(val);
    if (!ins)
      return true;

    return ins->getOpcode() != llvm::Instruction::Add &&
           ins->getOpcode() != llvm::Instruction::Mul;
  }

  // Remove all old unused instructions
  void _cleanup_roots(std::set<llvm::Value *> roots) {
    while (!roots.empty())
      _cleanup(roots, *roots.begin());
  }

  void _cleanup(std::set<llvm::Value *> &roots, llvm::Value *node) {
    if (!llvm::isa<llvm::Instruction>(node))
      return;
    auto ins = llvm::dyn_cast<llvm::Instruction>(node);
    if (ins->getNumUses() > 0)
      return;

    std::vector<llvm::Value *> ops;
    for (std::size_t i = 0; i < ins->getNumOperands(); ++i)
      ops.push_back(ins->getOperand(i));

    // Remove item if not used
    ins->eraseFromParent();
    roots.erase(node);

    // Remove all non used of its children recursively
    for (auto op : ops)
      _cleanup(roots, op);
  }
};

} // namespace

void thb_run(llvm::Module &mod) {

  for (auto &fun : mod)
    for (auto &bb : fun) {
      THB thb(bb);
      thb.run();
    }
}
