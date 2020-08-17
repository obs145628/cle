'''
Tree-Matching implementation
Based on BURS (Bottom-Up Rewrite System)
Extension to the table_full version, with a smaller table
The table is compressed (fewer states needed) during the construction
This is because some states may never the i-th children of some specific op, so it's useless to keep a specific entry for it.
Furthermore, some columns / rows of the table may be the same, and can be combined
This compressing is done while building the table.

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

class Representer:

    def __init__(self, idx):
        self.idx = idx
        self.rc = dict()

    def add_rule(self, r, cost):
        if r in self.rc and self.rc[r] <= cost:
            return False

        self.rc[r] = cost
        return True

    def get_match(self, r):
        return self.rc[r] if r in self.rc else MAX_COST

    def normalize(self):
        delta = MAX_COST
        for cost in self.rc.values():
            delta = min(delta, cost)

        for r in self.rc.keys():
            self.rc[r] -= delta

    def is_same_than(self, other):
        return self.rc == other.rc

    def __str__(self):
        res = 'Rep #{}: {{'.format(self.idx)
        for r in self.rc.keys():
            res += '({}, {}) '.format(r, self.rc[r])
        res += '}'
        return res


class OpTable:

    def __init__(self, name, arity):
        self.name = name
        self.arity = arity


        # defined for each position in op (size is arity)
        # list of representer states (only contain rules that can be used at pos i for this op
        self.reps = list()

        # defined for each position in op (size is arity)
        # mapping from state to representer state
        self.i_map = list() 

        # Object used to build the transition table
        # Mapping from representer states of children ops to global state of parent
        # n-dimensional array, with n = arity
        # trans Cannot be directly built because number of representer states is unknown
        # built from b_trans at the end
        self.b_trans = list()
        self.trans = None

        for i in range(self.arity):
            self.reps.append([])
            self.i_map.append(dict())
        

    def add_unique_rep(self, i, rep):
        for other in self.reps[i]:
            if other.is_same_than(rep):
                return (other, False)

        rep.idx = len(self.reps[i])
        self.reps[i].append(rep)
        return (rep, True)

    def add_st2rep_map(self, i, st, rep):
        self.i_map[i][st] = rep

    def add_trans(self, reps, new_st):
        self.b_trans.append((reps, new_st))


    def build_trans(self):
        for i in range(self.arity):
            print('Map for {}:{}:'.format(self.name, i))
            for r in self.i_map[i].keys():
                print('State #{} => Rep #{}'.format(r.idx, self.i_map[i][r].idx))
        print('')
            
        if self.arity == 1:
            self.build_trans_d1()
        elif self.arity == 2:
            self.build_trans_d2()

    def build_trans_d1(self):
        n = len(self.reps[0])
        self.trans = [None] * n

        for (reps, new_st) in self.b_trans:
            self.trans[reps[0].idx] = new_st

        print('Transition table for {}'.format(self.name))
        for i, st in enumerate(self.trans):
            print('#{}: state #{}'.format(i, st.idx))
        print('')

        for x in self.trans:
            assert x is not None

    def build_trans_d2(self):
        m = len(self.reps[0])
        n = len(self.reps[1])
        self.trans = [[None] * n for _ in range(m)]

        for (reps, new_st) in self.b_trans:
            self.trans[reps[0].idx][reps[1].idx] = new_st


        print('Transition table for {}'.format(self.name))
        sys.stdout.write('####| ')
        for i in range(len(self.trans[0])):
            sys.stdout.write('{:3}| '.format(i))
        print('')
        for i, row in enumerate(self.trans):
            sys.stdout.write('#{:3}| '.format(i))
            for st in row:
                sys.stdout.write('{:3}| '.format(st.idx))
            print('')
        print('')

        for x in self.trans:
            for y in x: assert y is not None

    def get_state(self, children_sts):
        reps = [self.i_map[i][children_sts[i]].idx for i in range(len(children_sts))]
        
        if self.arity == 1:
            return self.trans[reps[0]]
        elif self.arity == 2:
            return self.trans[reps[0]][reps[1]]
        else:
            assert 0

class TableBuilder:

    def __init__(self, rs):
        self.rs = rs
        self.ms = []
        self.leaf_ms = dict()
        self.wlist = []
        self.ops = {op[0]: OpTable(op[0], op[1]) for op in self.list_ops()}
        #self.init_ttrans()

    '''
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
    '''

    def project(self, st, op, i):
        rep = Representer(-1)
        for r in self.rs.rules:
            if not r.is_op() or op != r.get_op():
                continue

            n = r.get_args()[i]
            n_cost = st.get_match(n)[1]
            if n_cost != MAX_COST:
                rep.add_rule(n, n_cost)

        rep.normalize()
        return rep
        

    def build(self):
        self.compute_leaf_states()

        print('Initial states:')
        for leaf in self.list_leaves():
            print('{}: {}'.format(leaf, self.leaf_ms[leaf]))
            
        while len(self.wlist) != 0:
            self.update(self.wlist.pop())


        for op in self.ops.values():
            op.build_trans()
            
        #self.build_ttrans()
        
        

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
            self.compute_transitions(op[0], st)

    def compute_transitions(self, op, st):
        op = self.ops[op]
        arity = op.arity
        if arity == 0:
            return

        for i in range(0, arity):
            pstate = self.project(st, op.name, i)
            pstate, is_new = op.add_unique_rep(i, pstate)
            op.add_st2rep_map(i, st, pstate)
            if not is_new:
                continue

            if arity == 1:
                self.update_op_from_reps(op, [pstate])
            elif arity == 2 and i == 0:
                for other in op.reps[1]:
                    self.update_op_from_reps(op, [pstate, other])
            elif arity == 2 and i == 1:
                for other in op.reps[0]:
                    self.update_op_from_reps(op, [other, pstate])
            else:
                assert 0
                

    def update_op_from_reps(self, op, reps):
        new_st = State(-1)
        for r in self.rs.rules:
            if r.is_nt() or r.get_op() != op.name:
                continue

            cost = r.cost
            for (arg_i, arg) in enumerate(r.get_args()):
                arg_cost = reps[arg_i].get_match(arg)
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
            print('New state for {}({}): {}'.format(op.name, ', '.join([str(r.idx) for r in reps]), new_st))


        op.add_trans(reps, new_st)


    def get_state(self, op, children_sts):
        if len(children_sts) == 0:
            return self.leaf_ms[op]
        else:
            return self.ops[op].get_state(children_sts)
    
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
