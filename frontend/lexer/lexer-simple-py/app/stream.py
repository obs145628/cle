import alphabet

# FileStream to read file char by char
class Stream:


    def __init__(self, path):
        with open(path, 'r') as f:
            self.data = str(f.read())
        self.pos = 0
        self.extra = []

    # Get one character and char position
    def getc(self):
        if len(self.extra) > 0:
            return self.extra.pop()

        if self.pos == len(self.data):
            return alphabet.EOF

        res = self.data[self.pos]
        self.pos += 1
        return res

    # Add an extra char c to the stream
    # It will be the next one to be returned with getc
    # Can be called many times
    def ungetc(self, c):
        self.extra.append(c)
    
        
