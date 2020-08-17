import os

import graphivz

from digraph import Digraph

class Node:
    def __init__(self, idx, op, succs):
        self.idx = idx
        self.op = op
        self.pred = None
        self.succs = succs

class OpTree:

    def __init__(self):
        self.n2i = dict()
        self.i2n = list()
        self.nodes = list()
        self.root = None

    def build_arg(self, name):
        if name not in self.n2i:
            ty = 'Reg' if name.isalpha() else 'Int'
            self.add(name, ty, [])
        return self.nodes[self.n2i[name]]

    def add(self, name, op, args):
        args = [self.build_arg(a) for a in args]
        idx = len(self.i2n)
        self.i2n.append(name)
        self.n2i[name] = idx
        
        node = Node(idx, op, args)
        for a in args:
            assert a.pred is None
            a.pred = node
        self.nodes.append(node)

    def finish(self):
        for name in self.n2i.keys():
            node = self.nodes[self.n2i[name]]
            assert node.pred is not None or name == '@'
            if name == '@':
                self.root = node
        assert self.root is not None
        

    def save_dot(self, dot_file):
        assert self.root is not None
        g = Digraph(len(self.nodes))
        for n in self.nodes:
            g.set_vertex_label(n.idx, n.op)
            if n.pred is not None:
                g.add_edge(n.pred.idx, n.idx)

        g.save_dot(dot_file)

def parse_op(t, l):
    l = l.strip()
    if len(l) == 0:
        return

    ndef, op = [x.strip() for x in l.split('=')]
    op = [x.strip() for x in op.split(' ')]
    t.add(ndef, op[0], op[1:])

def parse_file(path):
    t = OpTree()
    with open(path, 'r') as f:
        for l in f.readlines():
            parse_op(t, l)
    t.finish()
    return t

if __name__ == '__main__':
    t = parse_file(os.path.join(os.path.dirname(__file__), '../examples/ex1.ir'))
    graphivz.show_obj(t)
