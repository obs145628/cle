
class Digraph:

    def __init__(self, v):
        self.v = v
        self.e = 0
        self.adj = [0] * (self.v * self.v)
        self.vertices_label = [str(u) for u in self.vertices()]

    def add_edge(self, u, v):
        self.adj[u * self.v + v] = 1

    def del_edge(self, u, v):
        self.adj[u * self.v + v] = 0

    def has_edge(self, u, v):
        return self.adj[u * self.v + v] == 1

    def set_vertex_label(self, u, label):
        self.vertices_label[u] = label

    def vertices(self):
        return range(0, self.v)

    def preds(self, u):
        return [v for v in self.vertices() if self.has_edge(v, u)]
    
    def succs(self, u):
        return [v for v in self.vertices() if self.has_edge(u, v)]


    def save_dot(self, dot_file):
        with open(dot_file, 'w') as f:
            f.write('digraph G{\n')

            for v in self.vertices():
                f.write('  {} [ label="{}" ];\n'.format(v, self.vertices_label[v]))

            for u in self.vertices():
                for v in self.succs(u):
                    f.write('  {} -> {}\n'.format(u, v))

            f.write('}\n')

    
