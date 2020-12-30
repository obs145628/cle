import os

import config

# Static DFA
# Deterministic Finite Automaton
# One start state, multiple final states, and one error state
# Can only add transitions
class DFA:

    def __init__(self, alpha, states_count, start_state, final_states, err_state):
        self.alpha = alpha
        self.sc = states_count
        # adjency matrix for transitions
        # each entry is a set of chars
        self.adj = [set() for _ in range(self.sc*self.sc)]
        self.start = start_state
        self.finals = final_states
        self.err = err_state

        # transition matrix
        # trans[i][c] = j means consume char c from state i lead to state j
        self.trans = [dict() for _ in range(self.sc)] 

    # transition from state s0 to s1 with char c
    # c can be None for spontaneaous transitions
    def add_transition(self, s0, s1, c):
        assert c not in self.trans[s0] #define every transition only once
        self.trans[s0][c] = s1

    # doesn't write error state to limit number of transitions
    def save_dot(self, out_path):

        ENTRY = '__entry'
        
        with open(out_path, 'w') as f:
            f.write('digraph G {\n')
            f.write("rankdir=LR;\n")
            f.write('size="8,5"\n')
            f.write('node [shape = point ]; {};\n'.format(ENTRY))

            # Define states
            for s in range(self.sc):
                if s == self.err: #ignore error state
                    continue
                if s in self.finals:
                    lbl = '{}:{}'.format(s, self.finals[s])
                    f.write('node [shape = doublecircle]; ')
                else:
                    lbl = s
                    f.write('node [shape = circle]; ')
                f.write('{} [ label="{}" ];\n'.format(s, lbl))
                

            # Draw transition for entry state
            f.write('{} -> {};\n'.format(ENTRY, self.start))
                
            #draw other transitions
            for s0 in range(self.sc):
                for s1 in range(self.sc):
                    if s0 == self.err or s1 == self.err or len(self.trans[s0]) == 0:
                        continue
                    es = [c for c in self.alpha.larr if self.trans[s0][c] == s1]
                    if len(es) == 0:
                        continue
                    has_esp = None in es
                    if has_esp:
                        es.remove(None)
                    es_text = self.alpha.range_repr(es)
                    if has_esp:
                        es_text = "<>" + es_text
                    f.write('{} -> {} [ label="{}" ];\n'.format(s0, s1, es_text))

            
                
            f.write('}\n')


    def logia_doc_write(self, doc):
        if config.HIDE_DOT:
            doc.write("DFA: {} states\n".format(self.sc))
            return
        
        fname = doc.gen_file_name()
        dot_path = os.path.join(doc.out_dir(), fname + ".dot")
        self.save_dot(dot_path)
        doc.img("NFA", doc.get_file_path(fname + ".png"))

    # Check if the DFA is valid
    def check(self):

        # check if there is a transition for all char in every states
        # and only for chars
        for tr in self.trans:
            if len(tr) > 0: #ignore removed states
                assert sorted(self.alpha.larr) == sorted(tr.keys())
                
        # check if err state only lead to err state
        err_tr = self.trans[self.err]
        for s in err_tr.values():
            assert s == self.err

        # check if final state is empty
        # it doesn't check if reachable
        assert len(self.finals) > 0
        

    # returns True if s matches the DFA
    def match(self, s):
        state = self.start
        for c in s:
            state = self.trans[state][c]
        return state in self.finals
        
