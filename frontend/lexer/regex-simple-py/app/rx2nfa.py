from consnfa import ConsNFA
import config

from logia.mdgfmdoc import MdGfmDoc
from logia.program import Program


class Converter:

    def __init__(self):
        self.nfa = ConsNFA()

    def visit(self, node):
        meth = getattr(self, 'visit_{}'.format(node.tname()))
        return meth(node)

    def build(self, node):
        tail, head = self.visit(node)
        self.nfa.finish(tail, head)
        nfa = self.nfa.build()

        doc = Program.instance().add_doc(MdGfmDoc, "Regex to NFA")
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
    

def convert(rx):
    conv = Converter()
    nfa = conv.build(rx)
    return nfa
