from dotgraph import DotGraph

import config

from logia.mdgfmdoc import MdGfmDoc
from logia.program import Program

'''
Syntax:
Base:
- a: match a chatachter a
- "abcd": match a then b then c then d
- \eps: match the empty string (epsilon)
- M|N: match regex M or N
- MN: match regex M, then regex N
- M*: match M zero or more times

Shorcuts:
- M+ -> MM*
- M? -> M|\eps
- [a-d46-8] -> a|b|c|d|4|6|7|8
- [^...] -> opposite of range
- "." -> c_0|c_1|..|c_n with c_1..c_n all chars in alphabet
- ":" :id ":" replace by range of chars for classname id

// operators precedence:
1: unary LTR: *, ?, +
2: binary LTR: MN
3: binary LTR M|N
'''

SPECIALS = ['[', ']', '(', ')', '*', '|', '+', '?', '.', '\\', ':', '"']

class NodeConcat:
    def __init__(self, left, right):
        self.left = left
        self.right = right

    def tname(self): return 'concat'

    def clone(self):
        return NodeConcat(self.left.clone(), self.right.clone())

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

    def tname(self): return 'or'

    def clone(self):
        return NodeOr(self.left.clone(), self.right.clone())

    def to_dot(self, g):
        l = self.left.to_dot(g)
        r = self.right.to_dot(g)
        op = g.add_vertex('|')
        g.add_edge(op, l)
        g.add_edge(op, r)
        return op

class NodeRange:
    def __init__(self, chars, alpha):
        self.chars = list(sorted(set(chars)))
        self.alpha = alpha

    def tname(self): return 'range'

    def clone(self):
        return NodeRange(self.chars, self.alpha)

    def to_dot(self, g):
        op = g.add_vertex('[{}]'.format(self.alpha.range_repr(self.chars)))
        return op

class NodeStar:
    def __init__(self, child):
        self.child = child

    def tname(self): return 'star'

    def clone(self):
        return NodeStar(self.child.clone())

    def to_dot(self, g):
        child = self.child.to_dot(g)
        op = g.add_vertex('*')
        g.add_edge(op, child)
        return op

class NodeEps:
    def __init__(self):
        pass

    def tname(self): return 'eps'

    def clone(self):
        return NodeEps()

    def to_dot(self, g):
        op = g.add_vertex('EPS')
        return op
    
    

class Regex:

    def __init__(self, rx_str, alpha):
        self.rx_str = "".join(rx_str.split())
        self.pos = 0
        self.alpha = alpha
        self.root = None
        self.build()

    def build(self):
        self.root = self.r_root()

        doc = Program.instance().add_doc(MdGfmDoc, "regex {}".format(self.rx_str))

        g = DotGraph(directed=True)
        self.root.to_dot(g)

        doc.write('```\n{}\n```\n'.format(self.rx_str))
        doc.h1("Regex Tree")
        doc.write(g)

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

    '''
    op1 -> prim ("*" | "?" | "+")*
    '''
    def r_op1(self):
        res = self.r_prim()
        while self.peekc() in ['*', '?', '+']:
            c = self.getc()

            if c == '*':
                res = NodeStar(res)
            elif c == '?':
                res = NodeOr(res, NodeEps())
            elif c == '+':
                res = NodeConcat(res, NodeStar(res.clone()))
            else:
                assert 0
            

        return res

    '''
    prim -> "(" exp ")"
    prim -> range
    prim -> special
    prim -> :char with :char in alphabet and :char not in SPECIALS (reserved symbols)
    prim -> "."
    prim -> quote
    prim -> classname
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

        if c == '\\':
            return self.r_special()


        if c in self.alpha.larr and c not in SPECIALS:
            self.getc()
            return NodeRange([c], self.alpha)

        if c == ".":
            self.getc()
            return NodeRange(self.alpha.larr, self.alpha)

        if c == "\"":
            return self.r_quote()

        if c == ':':
            return self.r_classname()
        
        assert 0
        

    '''
    range -> "[" [^] range-entry* "]"
    range-entry -> :char
    range-entry -> :char "-" :char
    '''
    def r_range(self):
        assert self.getc() == '['
        inv = False
        if self.peekc() == '^':
            self.getc()
            inv = True
        chars = []

        while self.peekc() != ']':
            c1 = self.getc()
            assert c1 in self.alpha.larr

            if self.peekc() == '-':
                self.getc()
                c2 = self.getc()
                assert c2 in self.alpha.larr and c2 not in SPECIALS
                chars += self.alpha.get_range(c1, c2)
            else:
                chars.append(c1)
        
        assert self.getc() == ']'
        if inv:
            chars = self.alpha.inv_chars(chars)
        return NodeRange(chars, self.alpha)

    '''
    special -> \eps
    '''
    def r_special(self):
        assert self.getc() == '\\'
        assert self.getc() == 'e'
        assert self.getc() == 'p'
        assert self.getc() == 's'
        return NodeEps()


    '''
    quote -> '"' :id '"'
    '''
    def r_quote(self):
        assert self.getc() == '"'
        assert self.peekc() != '"'
        left = None
        while self.peekc() != '"':
            
            c = self.getc()
            if c == '\\':
                qual = self.getc()
                if qual == '\\':
                    c == '\\'
                elif qual == 'n':
                    c == '\n'
                elif qual == 't':
                    c == '\t'
                elif qual == 'r':
                    c == '\r'
                elif qual == '"':
                    c == '"'
                else:
                    assert 0
            
            right = NodeRange([c], self.alpha)
            if left is None:
                left = right
            else:
                left = NodeConcat(left, right)
        assert self.getc() == '"'

        return left
    
    
    '''
    classname -> ":" :id ":"
    '''
    def r_classname(self):
        assert self.getc() == ':'
        name = ""
        while self.peekc() != ':':
            name += self.getc()
        assert self.getc() == ':'

        return NodeRange(self.alpha.get_class(name), self.alpha)
    
