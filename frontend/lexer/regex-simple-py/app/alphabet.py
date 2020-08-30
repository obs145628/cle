
LOWER = [chr(i) for i in range(ord('a'), ord('z') + 1)]
UPPER = [chr(i) for i in range(ord('A'), ord('Z') + 1)]
NUM = [chr(i) for i in range(ord('0'), ord('9') + 1)]
ALPHA = LOWER + UPPER
ALPHANUM = ALPHA + NUM

DICTS = {
    'ab': ['a', 'b'],
    'hexa': ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'],
    'alphanum': ALPHANUM,
}


class Alphabet:

    def __init__(self, letters):
        self.larr = list(letters)
        self.lmap = { l: idx for (idx, l) in enumerate(letters) }

    def get_range(self, c1, c2):
        c1 = self.lmap[c1]
        c2 = self.lmap[c2]
        assert c1 <= c2
        return [self.larr[i] for i in range(c1, c2+1)]
            

    @staticmethod
    def load(name):
        return Alphabet(DICTS[name])
        

if __name__ == '__main__':
    alpha = Alphabet.load('hexa')
    print(alpha.larr)
    print(alpha.lmap)
