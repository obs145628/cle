class Rule:

    def __init__(self, lhs, rhs, cost):
        self.idx = None
        self.lhs = lhs
        self.rhs = rhs
        self.cost = cost

    # RHS is a non-terminal
    def is_nt(self):
        return isinstance(self.rhs, str)

    # RHS if of of the form [op, nt_1, nt_2, ..., nt_n]
    def is_op(self):
        return not self.is_nt()

    def get_op(self):
        assert self.is_op()
        return self.rhs[0]

    def get_args(self):
        assert self.is_op()
        return self.rhs[1:]

    def __str__(self):
        if self.is_nt():
            return '#{}: {} -> {} ({})'.format(self.idx, self.lhs, self.rhs, self.cost)
        else:
            return '#{}: {} -> {}({}) ({})'.format(self.idx, self.lhs, self.get_op(),
                                              ', '.join(self.get_args()), self.cost)

class Rules:

    def __init__(self):
        self.rules = []
        self.next_gen = 0

    def add(self, r):
        r.idx = len(self.rules)
        self.rules.append(r)
        

    def gen_name(self, prefix):
        res = '{}_{}'.format(prefix, self.next_gen)
        self.next_gen += 1
        return res

    def __str__(self):
        res = ''
        for r in self.rules:
            res += '{}\n'.format(r)
        return res

class CharStream:

    def __init__(self, data):
        self.data = data
        self.pos = 0

    def peek(self):
        if self.pos == len(self.data):
            return '#'
        return self.data[self.pos]

    def get(self):
        if self.pos == len(self.data):
            return '#'
        self.pos += 1
        return self.data[self.pos - 1]
    
def parse_rhs(rules, lhs, cost, rhs):
    #parse op-name
    op = ""
    while rhs.peek().isalpha():
        op += rhs.get()
    assert len(op) != 0

    if rhs.peek() != '(':
        rules.add(Rule(lhs, [op], cost))
        return
    
    rhs.get()
    args = []
    

    # Parse args
    while rhs.peek() != ')':

        name = ""
        if rhs.peek().isupper():
            name = rules.gen_name(lhs)
            parse_rhs(rules, name, 0, rhs)
        else:
            while rhs.peek().isalpha():
                name += rhs.get()
            assert len(name) != 0
            
        args.append(name)

        if rhs.peek() == ',':
            rhs.get()
    rhs.get()
            
    rules.add(Rule(lhs, [op] + args, cost))
    
    

def parse_rule(l, rules):
    l = l.strip()
    if len(l) == 0:
        return

    fields = [x.strip() for x in l.split(';')]
    assert len(fields) == 3

    lhs = fields[0]
    rhs = fields[1].replace(' ', '')
    cost = int(fields[2])

    if rhs[0].islower():
        rules.add(Rule(lhs, rhs, cost))
    else:
        rhs = CharStream(rhs)
        parse_rhs(rules, lhs, cost, rhs)
        assert rhs.get() == '#'


def parse_file(path):
    res = Rules()
    with open(path, 'r') as f:
        for l in f.readlines():
            parse_rule(l, res)
    return res


if __name__ == '__main__':
    import os
    
    rules = parse_file(os.path.join(os.path.dirname(__file__), '../config/rules.txt'))
    print(rules)
