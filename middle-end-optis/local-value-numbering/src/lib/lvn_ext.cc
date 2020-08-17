#include "lvn.hh"

#include <utils/str/format-string.hh>

#include <cassert>
#include <iostream>
#include <map>

using val_t = std::size_t;

namespace {

bool is_const(const std::string &str) {
  if (str.empty())
    return false;
  if (str[0] == '-')
    return is_const(str.substr(1));
  return str[0] >= '0' && str[0] <= '9';
}

int get_const(const std::string &str) {
  std::size_t nb;
  int res = std::stoi(str, &nb, 10);
  assert(nb == str.size());
  return res;
}

bool is_reg(const std::string &str) { return str.size() > 1 && str[0] == '%'; }

template <class U, class V> void dump_map(const std::map<U, V> &m) {
  std::cerr << "Map {\n";
  for (auto &it : m)
    std::cerr << "  '" << it.first << "' => '" << it.second << "',\n";
  std::cerr << "}\n";
}

struct LVN {
  void run(Module &mod) {
    next_val = 0;

    for (std::size_t i = 0; i < mod.code.size(); ++i) {
      auto &ins = mod.code[i];
      const auto &opname = ins.args.front();

      if (opname == "add" || opname == "sub" || opname == "mul") {
        bool comut_op = opname == "add" || opname == "mul";
        auto &op_dst = ins.args[1];
        auto &op_s1 = ins.args[2];
        auto &op_s2 = ins.args[3];
        assert(is_reg(op_dst));

        // Replace operands regs with constants if possible
        op2const(op_s1);
        op2const(op_s2);

        // Perform constant folding
        if (is_const(op_s1) && is_const(op_s2)) {
          auto c1 = get_const(op_s1);
          auto c2 = get_const(op_s2);

          int res;
          if (opname == "add")
            res = c1 + c2;
          else if (opname == "sub")
            res = c1 - c2;
          else if (opname == "mul")
            res = c1 * c2;
          else
            assert(0);

          Ins new_ins;
          new_ins.args = {"mov", op_dst, std::to_string(res)};
          ins = new_ins;

          --i;
          continue;
        }

        // Apply algebric identiies
        if (apply_alg_ids(ins)) {
          --i;
          continue;
        }

        // Compute hash value of src operands
        auto s1h = get_op_hash(op_s1);
        auto s2h = get_op_hash(op_s2);

        // Compute hash value of dst by sorting operands if operation is
        // commumative This way, comutative exps like a+b and b+a get the same
        // hash key
        auto exp_hash = !comut_op || s1h < s2h
                            ? FORMAT_STRING(opname << ',' << s1h << ',' << s2h)
                            : FORMAT_STRING(opname << ',' << s2h << ',' << s1h);
        auto it = hash_vals.find(exp_hash);

        // Replace expression with mov
        // only possible if:
        // - expression was seen before
        // - there is a register with same value as expression value
        if (it != hash_vals.end() &&
            hash_inv.find(it->second) != hash_inv.end()) {
          // expression already computed
          auto cpy_reg = hash_inv.find(it->second)->second;
          Ins new_ins;
          new_ins.args = {"mov", op_dst, cpy_reg};
          ins = new_ins;

          --i;
          continue;
        }

        // new expression, add to hash table
        val_t ins_val = next_val++;
        udp_map(exp_hash, ins_val);
        udp_map(op_dst, ins_val);
      }

      else if (opname == "mov") {
        auto &op_dst = ins.args[1];
        auto &op_src = ins.args[2];
        assert(is_reg(op_dst));

        if (is_reg(op_src)) {
          // store src and dst in hash table
          auto srcv = get_op_val(op_src);
          udp_map(op_dst, srcv);
        }

        else if (is_const(op_src)) {
          // Assign a new number to dst
          auto dstv = next_val++;
          udp_map(op_dst, dstv);
          consts.emplace(dstv, get_const(op_src));
        }

        else {
          assert(0);
        }

        // Perform const transformation at the end
        // Avoid registering twice the same constant
        // This doesn't prevent the case where code contains twice the same
        // const Other solution would be to check if const already exists (with
        // double-side map)
        op2const(op_src);
      }

      else {
        assert(0);
      }
    }
  }

  // Get a value number for an operand
  // Create a new one if doesn't exit yet
  val_t get_op_val(const std::string &op) {
    if (hash_vals.find(op) == hash_vals.end())
      udp_map(op, next_val++);
    return hash_vals[op];
  }

  void udp_map(const std::string &key, val_t new_val) {
    auto it = hash_vals.find(key);

    if (it != hash_vals.end()) {
      // remove from inverse mapping if inv[vals[key]] == key
      auto inv_it = hash_inv.find(it->second);
      if (inv_it != hash_inv.end() && inv_it->second == key)
        hash_inv.erase(inv_it);
    }

    hash_vals[key] = new_val;

    // only store register hash in inverse mapping
    if (is_reg(key)) {
      hash_inv[new_val] = key;
    }
  }

  /// op is the source operand,
  /// if its a reg with a constant, replace it with the constant
  bool op2const(std::string &op) {
    if (!is_reg(op))
      return false;

    auto it = hash_vals.find(op);
    if (it == hash_vals.end())
      return false;

    auto const_it = consts.find(it->second);
    if (const_it == consts.end())
      return false;

    op = std::to_string(const_it->second);
    return true;
  }

  // Hash value of src operand
  std::string get_op_hash(const std::string &op) {
    if (is_reg(op)) {
      auto v = get_op_val(op);
      return "v" + std::to_string(v);
    } else if (is_const(op))
      return "c" + op;
    else
      assert(0);
  }

  // Apply common known algebric idendities (x*1, x+0, etc)
  // Long, repetitive error-prone implem
  // One solution is to use some form of pattern matching
  // eg: define a rule with 2 strings
  //   'add r:{d} r:{s} c:0' => 'mov {d} {s}'
  // from these rules find matches and apply transformations
  // another opti is to build a tree of transformations from these rules, to do
  // tree search instead of linear search to find a matching pattern
  //
  // Doesn't handle commutative ids (x*1, but not 1*x)
  bool apply_alg_ids(Ins &ins) {
    auto &args = ins.args;

    if (args[0] == "add") {
      if (is_reg(args[2]) && args[3] == "0") { // a + 0 => a
        args = {"mov", args[1], args[2]};
        return true;
      }

      else if (is_reg(args[2]) && args[2] == args[3]) { // a + a => 2 * a
        args = {"mul", args[1], "2", args[2]};
        return true;
      }
    }

    else if (args[0] == "sub") {
      if (is_reg(args[2]) && args[3] == "0") { // a - 0 => a
        args = {"mov", args[1], args[2]};
        return true;
      }

      else if (is_reg(args[2]) && args[2] == args[3]) { // a - a => 0
        args = {"mov", args[1], "0"};
        return true;
      }
    }

    else if (args[0] == "mul") {
      if (is_reg(args[2]) && args[3] == "0") { // a * 0 => 0
        args = {"mov", args[1], "0"};
        return true;
      }

      else if (is_reg(args[2]) && args[3] == "1") { // a + 1 => a
        args = {"mov", args[1], args[2]};
        return true;
      }
    }

    return false;
  }

  void dump() {
    std::cerr << "hash_vals: ";
    dump_map(hash_vals);
    std::cerr << "\nhash_inv: ";
    dump_map(hash_inv);
    std::cerr << "\nconsts: ";
    dump_map(consts);
    std::cerr << "\n";
  }

  // Mapping from an hashed key to its number
  // regs, constants and expression get hashed to unique values
  std::map<std::string, val_t> hash_vals;

  /// Inverse mapping to find the the register with the wanted value
  /// This isn't exactly like a double map, because many hash points to the
  /// same value This map hold the hash of regs only
  ///
  /// This inverse mapping isn't perfect, because it may only contain one reg
  /// per value Sometimes multiple reg aliases the same value If the reg in
  /// hash_inv change value, the value cannot be accessed anymore, even though
  /// other regs aliases it
  /// This could be fixed using a vector<string> of all regs aliasing the value
  /// for example, instead of storing just one
  std::map<val_t, std::string> hash_inv;

  // Store all value numbers with an associated constant integer
  // Used to perform constant folding
  std::map<val_t, int> consts;

  // Counter to assign different values
  val_t next_val;
};

} // namespace

void run_lvn_ext(Module &mod) {
  LVN lvn;
  lvn.run(mod);
  lvn.dump();
}
