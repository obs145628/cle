
# Find connected components in a digraph
class Connected:

    def __init__(self, g):
        self.g = g
        self.reprs = [None] * g.v
        self.count = 0
        self.build()

    def build(self):
        # Build representants for every node
        for v in range(self.g.v):
            if self.reprs[v] is None:
                self.count += 1
                self.dfs(v)

    def get_group(self, idx):
        return [v for v in self.g.vertices() if self.reprs[v] == idx]

    def dfs(self, v):
        self.reprs[v] = self.count - 1

        for w in self.g.succs(v):
            if self.reprs[w] is None:
                self.dfs(w)
