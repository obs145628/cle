import sys

from regex import Regex
from rules import Rules
from alphabet import Alphabet

import rx2nfa
import nfa2dfa
from dfamin import DFAMinimizer
from lexer import Lexer
from stream import Stream

if __name__ == '__main__':
    args = sys.argv
    if len(args) < 3:
        print('Usage: python app-lexer-simple.py <rules-path> <input-file>')
        sys.exit(1)

    alpha = Alphabet()
    rules_file = args[1]
    in_file = args[2]

    # parse all regexs
    rules = Rules(alpha, rules_file)

    # convert all regexs into one big NFA
    nfa = rx2nfa.convert(rules)

    # convert the NFA into a DFA
    dfa = nfa2dfa.convert(rules, nfa)

    # minimize the DFA
    dfa_min = DFAMinimizer(dfa, alpha)
    dfa_min.run()

    # lex file
    lexer = Lexer(dfa, Stream(in_file))
    while True:
        text, tok = lexer.get_tok()
        print('{}: `{}\''.format(tok, text))
        if tok == 'ERR' or tok == 'EOF':
            break

    
