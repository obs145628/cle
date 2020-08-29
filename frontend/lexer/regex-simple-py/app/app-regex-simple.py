import sys

from alphabet import Alphabet
from regex import Regex


if __name__ == '__main__':
    args = sys.argv
    if len(args) < 3:
        print('Usage: python app-regex-simple.py <alphabet> <regex>')
        sys.exit(1)

    alpha = Alphabet.load(args[1])    
    rx = Regex(args[2], alpha)
