from dotgraph import DotGraph

import config

from logia.mdgfmdoc import MdGfmDoc
from logia.program import Program

'''
Syntax:
Base:
- a: match a chatachter a
- \eps: match the empty string (epsilon)
- M|N: match regex M or N
- MN: match regex M, then regex N
- M*: match M zero or more times

Shorcuts:
- M+ -> MM*
- M? -> M|\eps
- [a-d46-8] -> a|b|c|d|4|6|7|8
- . -> c_0|c_1|..|c_n with c_1..c_n all chars in alphabet

// operators precedence:
1: unary RTL: *, ?, +
2: binary LTR: MN
3: binary LTR M|N
'''

SPECIALS = ['[', ']', '(', ')', '*', '|', '+', '?', '.']

class NodeConcat:
    def __init__(self, left, right):
        self.left = left
        self.right = right

    def to_dot(self, g):
        l = self.left.to_dot(g)
        r = self.right.to_dot(g)
        op = g.add_vertex('+')
        g.add_edge(op, l)
        g.add_edge(op, r)
        return op

class NodeOr:
    def __init__(self, left, right):
        self.left = left
        self.right = right

    def to_dot(self, g):
        l = self.left.to_dot(g)
        r = self.right.to_dot(g)
        op = g.add_vertex('|')
        g.add_edge(op, l)
        g.add_edge(op, r)
        return op

class NodeChar:
    def __init__(self, c):
        self.c = c

    def to_dot(self, g):
        op = g.add_vertex('[{}]'.format(self.c))
        return op

    

class Regex:

    def __init__(self, rx_str, alpha):
        self.rx_str = "".join(rx_str.split())
        self.pos = 0
        self.alpha = alpha

        self.root = self.r_root()

        self.doc = Program.instance().add_doc(MdGfmDoc, "regex")

        g = DotGraph(directed=True)
        self.root.to_dot(g)

        self.doc.write('```\n{}\n```\n'.format(self.rx_str))
        
        self.doc.h1("Regex Tree")
        self.doc.write(g)
        

    def peekc(self):
        if self.pos == len(self.rx_str):
            return None
        else:
            return self.rx_str[self.pos]

    def getc(self):
        res = self.peekc()
        if res is not None:
            self.pos += 1
        return res

    def r_root(self):
        res = self.r_exp()
        assert self.peekc() is None
        return res

    def r_exp(self):
        return self.r_op3()

    # op3 -> op2 (| op2)* 
    def r_op3(self):
        left = self.r_op2()
        while self.peekc() == '|':
            self.getc()
            right = self.r_op2()
            left = NodeOr(left, right)

        return left


    # op2 -> op1 op1*
    def r_op2(self):
        left = self.r_op1()
        while self.peekc() is not None and self.peekc() != ')' and self.peekc() != '|':
            right = self.r_op1()
            left = NodeConcat(left, right)

        return left

    def r_op1(self):
        return self.r_prim()

    '''
    prim -> "(" exp ")"
    prim -> range
    prim -> :char with :char in alphabet and :char not in SPECIALS (reserved symbols)
    '''
    def r_prim(self):
        c = self.peekc()
        assert c is not None
        
        if c == '(':
            self.getc()
            res = self.r_exp()
            assert self.getc() == ')'
            return res

        if c == '[':
            return self.r_range()


        if c in self.alpha.larr and c not in SPECIALS:
            self.getc()
            return NodeChar(c)
            
        
        assert 0
        

    def r_range(self):
        assert self.getc() == '['
        assert self.getc() == ']'
        return None



    
