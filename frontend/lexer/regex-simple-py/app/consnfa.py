from nfa import NFA

'''
NFA in construction
Vertices / edges are added dynamically
Edges may have no begin vertex, this is because the NFA is still being built
They will be connected later
'''
class ConsNFA:

    def __init__(self):
        self.v = 0
        self.edges = []
        self.start = None
        self.final = None

    def add_vertex(self):
        res = self.v
        self.v += 1
        return res

    # Edge from u to v with character c
    # u may be None
    # c is None means epsilon connection
    def add_edge(self, u, v, c=None):
        assert u >= 0 and u < self.v
        assert v >= 0 and v < self.v
        self.edges.append((u, v, c))

    # Edge to v, without starting vertex
    # c is None means epsilon connection
    def add_tail(self, v, c=None):
        assert v >= 0 and v < self.v
        res = len(self.edges)
        self.edges.append((None, v, c))
        return res

    # Connect tail e to some existing vertex v
    def connect_tail(self, e, v):
        old = self.edges[e]
        assert v >= 0 and v < self.v
        assert old[0] is None
        self.edges[e] = (v, old[1], old[2])

    # finish construction, set head as final state
    # and create a new state which the start state
    # to connect with tail
    def finish(self, tail, head):
        self.final = head
        self.start = self.add_vertex()
        self.connect_tail(tail, self.start)

    # convert to finished NFA
    def build(self):
        nfa = NFA(self.v, self.start, self.final)
        for e in self.edges:
            nfa.add_edge(e[0], e[1], e[2])
        return nfa

    def save_dot(self, out_path):

        ENTRY = '__entry'
        
        with open(out_path, 'w') as f:
            f.write('digraph G {\n')
            f.write("rankdir=LR;\n")
            f.write('size="8,5"\n')
            f.write('node [shape = point ]; {};\n'.format(ENTRY))

            # Define states
            for v in range(self.v):
                if v == self.final:
                    f.write('node [shape = doublecircle]; ')
                else:
                    f.write('node [shape = circle]; ')
                f.write('{};\n'.format(v))
                

            # Draw transition for entry state
            f.write('{} -> {};\n'.format(ENTRY, self.start))
                
            #draw other transitions
            for e in self.edges:
                lbl = "<>" if e[2] is None else e[2] #none is epsilon
                f.write('{} -> {} [ label="{}" ];\n'.format(e[0], e[1], lbl))

            
                
            f.write('}\n')
        
