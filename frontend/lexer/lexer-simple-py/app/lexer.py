from dfamatcher import DFAMatcher

class Lexer:

    def __init__(self, dfa, stream):
        self.dfa = DFAMatcher(dfa)
        self.stream = stream

    def get_tok(self):

        # Consume chars until an error state is reached
        self.dfa.reset()
        while True:
            self.dfa.consume(self.stream.getc())
            if self.dfa.is_err():
                break

        # Revert back to the first final state
        while not self.dfa.is_final() and not self.dfa.is_empty():
            self.stream.ungetc(self.dfa.back())

        # if dfa reverted to init state, there was no final state ever reached
        # returns an error
        if self.dfa.is_empty():
            return "", "ERR"

        # returns text and tag correponding to the last reached final state
        return self.dfa.get_text(), self.dfa.get_tag()
            
