from nfa import NFA

'''
NFA in construction
Vertices / edges are added dynamically
Edges may have no begin vertex, this is because the NFA is still being built
They will be connected later
'''
class ConsNFA:

    def __init__(self, alpha):
        self.alpha = alpha
        self.v = 0
        self.edges = []
        self.start = None
        self.finals = dict()

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

    # mark a vertex as a start
    # it can only be one
    def set_start(self, v):
        assert self.start is None
        self.start = v
        
    # mark a vertex as final
    # associate it a tag
    def set_final(self, v, tag):
        self.finals[v] = tag

    # convert to finished NFA
    def build(self):
        nfa = NFA(self.alpha, self.v, self.start, self.finals)
        for e in self.edges:
            nfa.add_edge(e[0], e[1], e[2])
        return nfa
        
