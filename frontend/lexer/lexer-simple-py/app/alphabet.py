
EOF = '\1'

LOWER = [chr(i) for i in range(ord('a'), ord('z') + 1)]
UPPER = [chr(i) for i in range(ord('A'), ord('Z') + 1)]
NUM = [chr(i) for i in range(ord('0'), ord('9') + 1)]
SPACES = [' ', '\n', '\r', '\t']
SYMS = ['_', '-', '+', '*', '/', '%', '=', '(', ')', '{', '}', '[', ']', '.', ',', ';', ':', '\\', '@', '#', '!', '?', '~', '>', '<']  
ALPHA = LOWER + UPPER
ALPHANUM = ALPHA + NUM
CHARS = ALPHANUM + SPACES + SYMS + [EOF]

class Alphabet:

    def __init__(self):
        self.larr = list(CHARS)
        self.lmap = { l: idx for (idx, l) in enumerate(self.larr) }

    def get_all(self):
        return list(self.larr)

    def get_range(self, c1, c2):
        c1 = self.lmap[c1]
        c2 = self.lmap[c2]
        assert c1 <= c2
        return [self.larr[i] for i in range(c1, c2+1)]

    def char_repr(self, c):
        if c == ' ':
            return ':s'
        elif c == '\n':
            return ':n'
        elif c == '\r':
            return ':r'
        elif c == '\t':
            return ':t'
        elif c == EOF:
            return 'EOF'
        else:
            return c

    def simplify_range(self, text, chars, cbeg, cend):
        rbeg = self.lmap[cbeg]
        rend = self.lmap[cend]

        if rbeg not in chars:
            return text, chars

        pos = chars.index(rbeg)
        count = rend - rbeg + 1
        for i in range(count):
            if chars[pos + i] != rbeg + i:
                return text, chars

        return "{}{}-{}".format(text, cbeg, cend), chars[:pos] + chars[pos+count:]
        
        
        

    def range_repr(self, chars):
        chars = sorted([self.lmap[c] for c in chars])
        if len(chars) == len(self.larr):
            return "."

        text = ""
        text, chars = self.simplify_range(text, chars, 'a', 'z')
        text, chars = self.simplify_range(text, chars, 'A', 'Z')
        text, chars = self.simplify_range(text, chars, '0', '9')

        return text + "".join([self.char_repr(self.larr[c]) for c in chars])
        
    def get_class(self, name):
        if name == 'space':
            return list(SPACES)
        elif name == 'eof':
            return [EOF]
        else:
            raise Exception('Unknown character classname {}'.format(name))
