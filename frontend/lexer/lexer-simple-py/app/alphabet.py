
EOF = '\1'

LOWER = [chr(i) for i in range(ord('a'), ord('z') + 1)]
UPPER = [chr(i) for i in range(ord('A'), ord('Z') + 1)]
NUM = [chr(i) for i in range(ord('0'), ord('9') + 1)]
SPACES = [' ', '\n', '\r', '\t']
SYMS = ['_', '-', '+', '*', '/', '%', '=', '(', ')', '{', '}', '[', ']', '.', ',', ';', ':', '\\', '@', '#', '!', '?', '~', '>', '<', '&', '|', '^', '"']  
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

    def inv_chars(self, chars):
        return [c for c in self.larr if c not in chars]
    
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

    def simplify_range_rec(self, text, chars, cbeg, cend):
        rbeg = self.lmap[cbeg]
        rend = self.lmap[cend]

        pos = 0
        while pos < len(chars) and chars[pos] < rbeg:
            pos += 1

        end_pos = pos
        while end_pos + 1 < len(chars) and chars[end_pos + 1] == chars[end_pos] + 1 and chars[end_pos + 1] <= rend:
            end_pos += 1

        if pos >= len(chars) or end_pos >= len(chars) or end_pos - pos < 3:
            return text, chars, False

        cfirst = self.larr[chars[pos]]
        clast = self.larr[chars[end_pos]]
        return "{}{}-{}".format(text, cfirst, clast), chars[:pos] + chars[end_pos+1:], True

    def simplify_range(self, text, chars, cbeg, cend):
        while True:
            text, chars, updated = self.simplify_range_rec(text, chars, cbeg, cend)
            if not updated:
                break
        return text, chars
        
        

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
