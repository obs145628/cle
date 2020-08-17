'''
Tree-Matching implementation
Based on BURS (Bottom-Up Rewrite System)
Similar to tree-match-burs1 project

Inspired from:
- Instruction Selection via Tree-Pattern Matching - Enginner a Compiler p610
- An Improvement to Bottom-up Tree Pattern Matching - David R. Chase
- Simple and Efficient BURS Table Generation - Todd A. Proebsting
'''


import os
import sys

import rules
import optree
from digraph import Digraph
import graphivz

MAX_COST = 100000000

class MatchInfos:


    def __init__(self, node):
        self.node = node

        # list of pairs: rule / minimum cost for rule matching that node
        self.rc = dict()

    def add_match(self, r, cost):
        name = r.lhs
        if name in self.rc and self.rc[name][0] <= cost:
            return False

        self.rc[name] = (r, cost)
        return True

    def get_match(self, rule_name):
        if rule_name in self.rc:
            return self.rc[rule_name]
        return (None, MAX_COST)

class Matcher:

    def __init__(self, rules, t):
        self.rules = rules
        self.t = t
        self.infos = [MatchInfos(n) for n in self.t.nodes]

    def match(self):
        self.match_node(t.root)

    def match_node(self, node):
        # Match children first
        for arg in node.succs:
            self.match_node(arg)


        for r in self.rules.rules:
            if r.is_nt():
                continue # non-terminat rules matched indirectly
            
            if r.get_op() != node.op or len(r.get_args()) != len(node.succs):
                continue # doesn't match

            cost = r.cost

            # try to match all children
            for (arg_i, arg) in enumerate(node.succs):

                arg_infos = self.infos[arg.idx]
                arg_rule = r.get_args()[arg_i]
                arg_cost = arg_infos.get_match(arg_rule)[1]

                if arg_cost == MAX_COST:
                    cost = MAX_COST
                    break
                cost += arg_cost

            if cost != MAX_COST:
                #natch found
                self.add_match(node, r, cost)
                
                

    def add_match(self, node, r, cost):
        infos = self.infos[node.idx]
        if not infos.add_match(r, cost):
            return

        # propagate infos to all non-terminal rules
        for ntr in self.rules.rules:
            if ntr.is_nt() and ntr.rhs == r.lhs:
                self.add_match(node, ntr, cost + ntr.cost)


    def apply(self, runner):
        self.apply_rec(runner, self.t.root, 'goal')

    def apply_rec(self, runner, node, rule):

        # Get node match rule
        match = self.infos[node.idx].get_match(rule)
        if match[0] == None:
            raise Exception('Matching tree failed')

        # List all non-terminal rules
        nt_rules = []
        while match[0].is_nt():
            nt_rules.append(match[0])
            match = self.infos[node.idx].get_match(match[0].rhs)
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

            for rc in infos.rc.values():
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
    print(rs)
    t = optree.parse_file(sys.argv[1])
    matcher = Matcher(rs, t)
    matcher.match()
    graphivz.show_obj(matcher.all_matches())
    graphivz.show_obj(matcher.apply_matches())
