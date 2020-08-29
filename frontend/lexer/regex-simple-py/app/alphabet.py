

DICTS = {
    'ab': ['a', 'b'],
    'hexa': ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'],
}


class Alphabet:

    def __init__(self, letters):
        self.larr = list(letters)
        self.lmap = { l: idx for (idx, l) in enumerate(letters) }

    @staticmethod
    def load(name):
        return Alphabet(DICTS[name])
        

if __name__ == '__main__':
    alpha = Alphabet.load('hexa')
    print(alpha.larr)
    print(alpha.lmap)
