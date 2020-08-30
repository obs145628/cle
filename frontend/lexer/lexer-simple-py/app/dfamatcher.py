

class DFAMatcher:

    def __init__(self, dfa):
        self.dfa = dfa
        self.reset()

    # reset matcher to start state (no chars)
    def reset(self):
        self.state = self.dfa.start
        self.hist = []

    def is_empty(self):
        return len(self.hist) == 0
        
    def is_final(self):
        return self.state in self.dfa.finals

    def get_tag(self):
        return self.dfa.finals[self.state]

    def is_err(self):
        return self.state == self.dfa.err


    # Consume one character and update state
    def consume(self, c):
        self.hist.append((self.state, c))
        self.state = self.dfa.trans[self.state][c]

    # cancel last consumed char and returns it
    def back(self):
        self.state, c = self.hist.pop()
        return c

    # get all consumed text to reach current state
    def get_text(self):
        return "".join([st[1] for st in self.hist])
