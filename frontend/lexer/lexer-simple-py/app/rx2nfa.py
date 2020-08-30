from consnfa import ConsNFA
import config

from logia.mdgfmdoc import MdGfmDoc
from logia.program import Program


class Converter:

    def __init__(self, rules):
        self.nfa = ConsNFA(rules.alpha)
        self.rules = rules

    

    def visit(self, node):
        meth = getattr(self, 'visit_{}'.format(node.tname()))
        return meth(node)

    def build_rule(self, rule):
        rx = rule[0]
        tag = rule[1]

        tail, head = self.visit(rx.root)
        self.nfa.set_final(head, tag)
        self.nfa.connect_tail(tail, self.start_state)

    def build(self):
        # create start state
        # all regex root tails will be connected to this one
        self.start_state = self.nfa.add_vertex()
        self.nfa.set_start(self.start_state)
        
        for rule in self.rules.rules:
            self.build_rule(rule)
            
        nfa = self.nfa.build()

        doc = Program.instance().add_doc(MdGfmDoc, "Regexs to NFA")
        doc.write(nfa)
        return nfa

    def visit_concat(self, node):
        t1, h1 = self.visit(node.left)
        t2, h2 = self.visit(node.right)
        self.nfa.connect_tail(t2, h1)
        return (t1, h2)

    def visit_or(self, node):
        t1, h1 = self.visit(node.left)
        t2, h2 = self.visit(node.right)

        head = self.nfa.add_vertex()
        self.nfa.add_edge(h1, head)
        self.nfa.add_edge(h2, head)

        first = self.nfa.add_vertex()
        self.nfa.connect_tail(t1, first)
        self.nfa.connect_tail(t2, first)
        tail = self.nfa.add_tail(first)
        return (tail, head)

    def visit_range(self, node):
        chars = node.chars
        assert len(chars) > 0
        head = self.nfa.add_vertex()
        
        if len(chars) == 1: # simpler solution if there is only one char
            tail = self.nfa.add_tail(head, chars[0])
            return (tail, head)

        beg = self.nfa.add_vertex()
        for c in chars:
            self.nfa.add_edge(beg, head, c)

        tail = self.nfa.add_tail(beg)
        return (tail, head)

    def visit_star(self, node):
        ct, ch = self.visit(node.child)

        head = self.nfa.add_vertex()
        self.nfa.add_edge(ch, head)
        self.nfa.connect_tail(ct, head)
        tail = self.nfa.add_tail(head)
        return (tail, head)

    def visit_eps(self, node):
        head = self.nfa.add_vertex()
        tail = self.nfa.add_tail(head)
        return (tail, head)
    

def convert(rules):
    conv = Converter(rules)
    nfa = conv.build()
    return nfa
