from connected import Connected
from graph import Graph
import config

from logia.mdgfmdoc import MdGfmDoc
from logia.program import Program

# The conversion NFA -> DFA gives a DFA with a lot of equivalent states
# This algorithm find pair of equivalent states
# Then it combines all equivalent states into one
class DFAMinimizer:

    def __init__(self, dfa, alpha):
        self.dfa = dfa
        self.alpha = alpha

        self.doc = Program.instance().add_doc(MdGfmDoc, "DFA Minimization")


    def run(self):
        self.find_pairs()
        self.find_equiv()
        self.simplify()

    # Find all pairs of equivalent states
    def find_pairs(self):

        # Compute inequivalent pairs
        # g has edge u-v iff u and v are inequivalent
        g = Graph(self.dfa.sc)

        # First mark states pair (s0, s1) inequivalent if one is final and not the other
        for s0 in range(self.dfa.sc):
            for s1 in range(self.dfa.sc):
                s0_final = s0 in self.dfa.finals
                s1_final = s1 in self.dfa.finals
                if s0_final != s1_final:
                    g.add_edge(s0, s1)

        # Iterate to find all inequivalent states
        while True:
            changed = False

            for s0 in range(self.dfa.sc):
                for s1 in range(s0+1, self.dfa.sc):
                    if g.has_edge(s0, s1): #already inequivalent
                        continue

                    for c in self.alpha.larr:
                        s0p = self.dfa.trans[s0][c]
                        s1p = self.dfa.trans[s1][c]
                        if g.has_edge(s0p, s1p):
                            #same trans, different state => inequivalent
                            changed = True
                            g.add_edge(s0, s1)
                            break

            if not changed:
                break

        # Reverse graph to get equivalent states
        g = g.reverse_adj()
        # ignore self-edges, they are not relevant
        for v in range(self.dfa.sc):
            g.del_edge(v, v)

        self.pairs_graph = g    
        self.doc.h1("Equivalence pairs")
        self.doc.write(g)


    def find_equiv(self):
        # Use connected components to find all equivalent states from pairs
        self.conn = Connected(self.pairs_graph)

        self.doc.h1('Equivalence states ({} states)'.format(self.conn.count))

        for i in range(self.conn.count):
            group = self.conn.get_group(i)
            self.doc.write('- `{')
            for s in group:
                self.doc.write('{}; '.format(s))
            self.doc.write('}`\n')

    def simplify(self):

        for i in range(self.conn.count):
            group = self.conn.get_group(i)
            if len(group) == 1:
                continue

            main = group[0]
            for s in group[1:]:
                self.replace_with(s, main)

        self.doc.h1('Minimized DFA')
        self.doc.write(self.dfa)
        self.dfa.check(self.alpha)
        

    # replace state s0 by state s1
    # replace all trans[s, c, s0] by [s, c, s1]
    # remove all trans [s0, c, c]
    # remove s1 from final state
    # if s0 is start, set s1 as start
    # remove state s1
    def replace_with(self, s0, s1):

        for s in range(self.dfa.sc):
            if s == s0 or len(self.dfa.trans[s]) == 0: #replaced state
                continue
            for c in self.alpha.larr:
                if self.dfa.trans[s][c] == s0:
                    self.dfa.trans[s][c] = s1

        self.dfa.trans[s0] = dict()

        if s0 in self.dfa.finals:
            self.dfa.finals.remove(s0)

        if self.dfa.start == s0:
            self.dfa.start = s1
