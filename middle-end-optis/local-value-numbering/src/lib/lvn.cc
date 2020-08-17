#include "lvn.hh"

#include <utils/str/format-string.hh>

#include <cassert>
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

bool is_reg(const std::string &str) { return str.size() > 1 && str[0] == '%'; }

struct LVN {
  void run(Module &mod) {
    next_val = 0;

    for (auto &ins : mod.code) {
      const auto &opname = ins.args.front();
      if (opname == "add" || opname == "sub") {
        auto lv = get_op_val(ins.args[2]);
        auto rv = get_op_val(ins.args[3]);
        auto exp_hash = FORMAT_STRING(opname << ",v" << lv << ",v" << rv);

        auto it = hash_vals.find(exp_hash);
        val_t ins_val;

        // Need to check 2 things:
        // - expression was seen before
        // - there is a register with this value
        if (it != hash_vals.end() &&
            hash_inv.find(it->second) != hash_inv.end()) {
          // expression already computed
          ins_val = it->second;
          Ins new_ins;
          auto cpy_reg = hash_inv.find(ins_val)->second;
          new_ins.args = {"mov", ins.args[1], cpy_reg};
          ins = new_ins;
        }

        else {
          // new expression, add to hash table
          ins_val = next_val++;
          udp_map(exp_hash, ins_val);
        }

        // Always had dst operand to hash table
        udp_map(ins.args[1], ins_val);
      }

      else if (opname == "mov") {
        auto op_dst = ins.args[1];
        auto op_src = ins.args[2];
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
        }

        else {
          assert(0);
        }
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

  // Mapping from an hashed key to its number
  // regs, constants and expression get hashed to unique values
  std::map<std::string, val_t> hash_vals;

  /// Inverse mapping to find the the register with the wanted value
  /// This isn't exactly like a double map, because many hash points to the
  /// same value This map hold the hash of regs only
  std::map<val_t, std::string> hash_inv;

  // Counter to assign different values
  val_t next_val;
};

} // namespace

void run_lvn(Module &mod) {
  LVN lvn;
  lvn.run(mod);
}
