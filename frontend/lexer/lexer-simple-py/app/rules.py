from regex import Regex

class Rules:

    def __init__(self, alpha, path):
        self.alpha = alpha
        self.rules = []

        with open(path, 'r') as f:
            for l in f.readlines():
                l = l.strip()
                if len(l) == 0:
                    continue
                l = l.split('=>')
                rx = Regex(l[0].strip(), self.alpha)
                tag = l[1].strip()
                self.rules.append([rx, tag])
        
