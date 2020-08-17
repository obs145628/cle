'''
Tree-Matching implementation
Based on BURS (Bottom-Up Rewrite System)
Contrary to naive version, builds a full table of all possible states in the naive version
The table allows to know, given the op and the state of the children operand,
what will be the state of the parent

Inspired from:
- Instruction Selection via Tree-Pattern Matching - Enginner a Compiler p610
- An Improvement to Bottom-up Tree Pattern Matching - David R. Chase
- Simple and Efficient BURS Table Generation - Todd A. Proebsting
'''

import os
import sys

import rules
import optree
import graphivz
from digraph import Digraph

MAX_COST = 100000000

class State:

    def __init__(self, idx):
        self.idx = idx
        self.rc = dict()

    def add_rule(self, r, cost):
        name = r.lhs
        if name in self.rc and self.rc[name][1] <= cost:
            return False

        self.rc[name] = (r, cost)
        return True

    def get_match(self, r):
        if r in self.rc:
            return self.rc[r]
        return (None, MAX_COST)

    def normalize(self):
        delta = MAX_COST
        for r in self.rc.values():
            delta = min(delta, r[1])

        for name in self.rc.keys():
            rc = self.rc[name]
            self.rc[name] = (rc[0], rc[1] - delta)

    def is_same_than(self, other):
        if len(self.rc) != len(other.rc):
            return False

        for name in self.rc:
            if name not in other.rc:
                return False
            self_rc = self.rc[name]
            other_rc = other.rc[name]
            if self_rc[0] != other_rc[0] or self_rc[1] != other_rc[1]:
                return False

        return True

    def __str__(self):
        res = 'State #{}: {{'.format(self.idx)
        for r in self.rc.values():
            res += '({}#{}, {}) '.format(r[0].idx, r[0].lhs, r[1])
        res += '}'
        return res
            

class TableBuilder:

    def __init__(self, rs):
        self.rs = rs
        self.ms = []
        self.leaf_ms = dict()
        self.wlist = []
        self.ops_arity = {op[0]: op[1] for op in self.list_ops()}
        self.init_ttrans()

    def init_ttrans(self):
        #Init transition tables
        self.ttrans = dict()
        for op in self.ops_arity.keys():
            if self.ops_arity[op] > 0:
                self.ttrans[op] = []

    def build_ttrans(self):
        for op in self.ttrans.keys():
            if self.ops_arity[op] == 1:
                self.ttrans[op] = self.build_ttrans_1(op)
            elif self.ops_arity[op] == 2:
                self.ttrans[op] = self.build_ttrans_2(op)
            else:
                assert 0

    def build_ttrans_1(self, op):
        data = self.ttrans[op]
        res = [None] * len(self.ms)
        for e in data:
            res[e[0][0].idx] = e[1]

        print('Transition table for {}'.format(op))
        for i, st in enumerate(res):
            print('#{}: state #{}'.format(i, st.idx))
        print('')
            

        for x in res: assert x is not None
        return res

    def build_ttrans_2(self, op):
        data = self.ttrans[op]
        res = [[None] * len(self.ms) for _ in self.ms]
        
        for e in data:
            res[e[0][0].idx][e[0][1].idx] = e[1]

        print('Transition table for {}'.format(op))
        sys.stdout.write('####| ')
        for i in range(len(res)):
            sys.stdout.write('{:3}| '.format(i))
        print('')
        for i, row in enumerate(res):
            sys.stdout.write('#{:3}| '.format(i))
            for st in row:
                sys.stdout.write('{:3}| '.format(st.idx))
            print('')
        print('')

        for x in res:
            for y in x: assert y is not None
        return res
        
        

    def build(self):
        self.compute_leaf_states()

        print('Initial states:')
        for leaf in self.list_leaves():
            print('{}: {}'.format(leaf, self.leaf_ms[leaf]))
            
        while len(self.wlist) != 0:
            self.update(self.wlist.pop())

        self.build_ttrans()
        
        

    def list_leaves(self):
        return [op[0] for op in self.list_ops() if op[1] == 0]

    def list_ops(self):
        ops = set()
        res = []
        for r in self.rs.rules:
            if r.is_op() and r.get_op() not in ops:
                ops.add(r.get_op())
                res.append((r.get_op(), len(r.get_args())))
        return res
        

    def add_state(self):
        idx = len(self.ms)
        st = State(idx)
        self.ms.append(st)
        return st

    def add_unique_state(self, new_st):
        for st in self.ms:
            if new_st.is_same_than(st):
                return (st, False)

        idx = len(self.ms)
        new_st.idx = idx
        self.ms.append(new_st)

        return (new_st, True)

    def compute_leaf_states(self):
        for leaf in self.list_leaves():
            self.compute_leaf_st(leaf)
    
    def compute_leaf_st(self, leaf):
        st = self.add_state()
        self.leaf_ms[leaf] = st
        
        for r in self.rs.rules:
            if r.is_op() and r.get_op() == leaf:
                st.add_rule(r, r.cost)

        self.closure(st)
        st.normalize()
        self.wlist.append(st)

    def closure(self, st):

        while True:
            changed = False

            for r in self.rs.rules:
                if not r.is_nt():
                    continue
                
                new_cost = r.cost + st.get_match(r.rhs)[1]
                if new_cost >= MAX_COST:
                    continue

                if st.add_rule(r, new_cost):
                    changed = True

            if changed == False:
                break


    def update(self, st):
        for op in self.list_ops():
            self.update_op(op[0], st)

    def update_op(self, op, st):
        arity = self.ops_arity[op]
        if arity == 0:
            return

        if arity == 1:
            return self.update_op_from_states(op, [st])

        assert arity == 2
            
        ms = list(self.ms)
        for other in ms:
            self.update_op_from_states(op, [st, other])
            if other != st:
                self.update_op_from_states(op, [other, st])

    def update_op_from_states(self, op, sts):
        new_st = State(-1)
        for r in self.rs.rules:
            if r.is_nt() or r.get_op() != op:
                continue

            cost = r.cost
            for (arg_i, arg) in enumerate(r.get_args()):
                arg_cost = sts[arg_i].get_match(arg)[1]
                if arg_cost == MAX_COST:
                    cost = MAX_COST
                    break
                cost += arg_cost
                
            if cost != MAX_COST:
                new_st.add_rule(r, cost)    

        self.closure(new_st)
        new_st.normalize()
        
        new_st, updated = self.add_unique_state(new_st)
        if updated:
            self.wlist.append(new_st)
            print('New state for {}({}): {}'.format(op, ', '.join([str(s.idx) for s in sts]), new_st))

        self.ttrans[op].append((sts, new_st))


    def get_state(self, op, children_sts):
        arity = self.ops_arity[op]
        if arity == 0:
            return self.leaf_ms[op]
        elif arity == 1:
            return self.ttrans[op][children_sts[0].idx]
        elif arity == 2:
            return self.ttrans[op][children_sts[0].idx][children_sts[1].idx]
        else:
            assert 0
    
class MatchInfos:


    def __init__(self, node):
        self.node = node
        self.state = None
        
                
class Matcher:

    def __init__(self, table, t):
        self.table = table
        self.t = t
        self.infos = [MatchInfos(n) for n in self.t.nodes]

    def match(self):
        self.match_node(t.root)

    def match_node(self, node):
        # Match children first
        for arg in node.succs:
            self.match_node(arg)


        infos = self.infos[node.idx]
        infos.state = self.table.get_state(node.op,
                                           [self.infos[arg.idx].state for arg in node.succs])


    def apply(self, runner):
        self.apply_rec(runner, self.t.root, 'goal')

    def apply_rec(self, runner, node, rule):

        # Get node match rule
        match = self.infos[node.idx].state.get_match(rule)
        if match[0] == None:
            raise Exception('Matching tree failed')

        # List all non-terminal rules
        nt_rules = []
        while match[0].is_nt():
            nt_rules.append(match[0])
            match = self.infos[node.idx].state.get_match(match[0].rhs)
            assert match[0] is not None

        
        # Apply non-terminal rules
        for r in nt_rules:
            runner.before(node, r)
        
        
        # Apply to all children
        rule = match[0]
        runner.before(node, rule)
        for (arg_i, arg) in enumerate(node.succs):
            arg_rule = rule.get_args()[arg_i]
            self.apply_rec(runner, arg, arg_rule)
        runner.after(node, rule)

        # Apply non-terminal rules
        for r in nt_rules[::-1]:
            runner.after(node, r)
        


    def all_matches(self):

        def get_label(infos):
            res = "{} {{".format(infos.node.op)

            for rc in infos.state.rc.values():
                res += "({}#{}, {}) ".format(rc[0].lhs, rc[0].idx, rc[1])

            res += "}"
            return res
        
        class Helper:
            def __init__(self, obj):
                self.obj = obj
            def save_dot(self, dot_file):
                g = Digraph(len(self.obj.infos))
                for infos in self.obj.infos:
                    n = infos.node
                    g.set_vertex_label(n.idx, get_label(infos))
                    if n.pred is not None:
                        g.add_edge(n.pred.idx, n.idx)

                g.save_dot(dot_file)

        return Helper(self)

    def apply_matches(self):
        class Helper:
            def __init__(self, obj):
                self.obj = obj
                self.labels = ['' for _ in self.obj.infos]

            def save_dot(self, dot_file):
                g = Digraph(len(self.obj.infos))
                for infos in self.obj.infos:
                    n = infos.node
                    if n.pred is not None:
                        g.add_edge(n.pred.idx, n.idx)

                self.obj.apply(self)

                for (u, label) in enumerate(self.labels):
                    g.set_vertex_label(u, label)
                    

                g.save_dot(dot_file)

            def before(self, node, rule):
                lbl = self.labels[node.idx]

                if len(lbl) == 0:
                    lbl = '{}: '.format(node.op)

                if not lbl.endswith(': '):
                    lbl += ' + '
                lbl += '{}#{}'.format(rule.lhs, rule.idx)

                self.labels[node.idx] = lbl

            def after(self, node, rule):
                pass

        return Helper(self)


if __name__ == '__main__':
    rs = rules.parse_file(os.path.join(os.path.dirname(__file__), '../config/rules.txt'))
    print('Rules:')
    print(rs)
    tb = TableBuilder(rs)
    tb.build()

    t = optree.parse_file(sys.argv[1])
    matcher = Matcher(tb, t)
    matcher.match()
    graphivz.show_obj(matcher.all_matches())
    graphivz.show_obj(matcher.apply_matches())
