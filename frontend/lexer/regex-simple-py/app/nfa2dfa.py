from dfa import DFA
import config

from logia.mdgfmdoc import MdGfmDoc
from logia.program import Program

class Converter:

    def __init__(self, nfa, alpha):
        self.nfa = nfa
        self.alpha = alpha

    # returns all states reachable through s by taking char c
    # is c is None, does one level of spontaneaous transitions
    def edge(self, s, c):
        res = set()
        for d in range(self.nfa.sc):
            if c in self.nfa.edges(s, d):
                res.add(d)
        return res

    
        
    # S set of states
    # returns all set reachables from any sate in S, using only spontaneaous transitions
    def closure(self, S):
        S = set(S)
        while True:
            copy = set(S)
            for s in copy:
                for x in self.edge(s, None):
                    S.add(x)

            if S == copy:
                break
        return S

    # let d a set of states in NFA (equivalent to one state in the DFA)
    # returns a new states of states in NFA (equivalent to one state in the DFA)
    # rechable from tacking char c from any state in d, then taking spontaneaous transitions
    def dfa_edge(self, d, c):
        succ = set()
        for s in d:
            for x in self.edge(s, c):
                succ.add(x)

        return self.closure(succ)

    def find_state(self, st):
        for i, other in enumerate(self.states):
            if other == st:
                return i
        return None
        

    def build(self):
        self.states = []
        self.doc = Program.instance().add_doc(MdGfmDoc, "NFA to DFA")
        self.doc.h1("DFA construction")

        # First state is empty
        # This is the error state
        # All invalid transitions are stuck in this state
        self.states.append(set())
        self.dump_state(0)

        # Second state is the eps-closure of the start state
        self.states.append(self.closure({self.nfa.start}))
        self.dump_state(1)
        
        self.trans_list = []
        j = 0
        while j < len(self.states):
            for c in self.alpha.larr:
                # compute new state from state j, taking char c
                e = self.dfa_edge(self.states[j], c)

                # check if same set of states already exists
                i = self.find_state(e)

                if i is None:
                    # add new state
                    i = len(self.states)
                    self.states.append(e)
                    self.dump_state(i)

                # record transition
                self.trans_list.append((j, c, i))

            j += 1

        # Compute list of final states
        # one state is final if any of it's NFA states is final
        self.finals = [d for (d, st) in enumerate(self.states) if self.nfa.final in st]
        self.dump_finals()

        # Build DFA
        self.dfa = DFA(len(self.states), start_state=1, final_states=self.finals, err_state=0)
        for t in self.trans_list:
            self.dfa.add_transition(s0=t[0], s1=t[2], c=t[1])

        self.doc.h1('DFA')
        self.doc.write(self.dfa)

        return self.dfa

    def dump_state(self, idx):
        st = self.states[idx]
        self.doc.write('- Inserted state {}: `{{'.format(idx))
        for s in st:
            self.doc.write('{}; '.format(s))
        self.doc.write('}`\n')

    def dump_finals(self):
        self.doc.write('- Final states: `{')
        for s in self.finals:
            self.doc.write('{}; '.format(s))
        self.doc.write('}`\n')
            


def convert(nfa, alpha):
    conv = Converter(nfa, alpha)
    return conv.build()
