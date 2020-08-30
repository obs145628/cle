import os

# Undirected graph
class Graph:

    def __init__(self, v):
        self.v = v
        self.adj = [0] * (self.v * self.v)

    def add_edge(self, u, v):
        self.adj[u * self.v + v] = 1
        self.adj[v * self.v + u] = 1

    def del_edge(self, u, v):
        self.adj[u * self.v + v] = 0
        self.adj[v * self.v + u] = 0

    def has_edge(self, u, v):
        return self.adj[u * self.v + v] == 1

    def vertices(self):
        return range(self.v)

    def succs(self, v):
        return [w for w in self.vertices() if self.has_edge(v, w)]

    def reverse_adj(self):
        rev = Graph(self.v)
        rev.adj = [int(not x) for x in self.adj]
        return rev
    

    def save_dot(self, dot_file):
        with open(dot_file, 'w') as f:
            f.write('graph G{\n')

            for v in range(self.v):
                f.write('  {} [ label="{}" ];\n'.format(v, v))


            for v0 in range(self.v):
                for v1 in range(v0, self.v):
                    if self.has_edge(v0, v1):
                        f.write('  {} -- {}\n'.format(v0, v1))

            f.write('}\n')



    def logia_doc_write(self, doc):
        fname = doc.gen_file_name()
        dot_path = os.path.join(doc.out_dir(), fname + ".dot")
        self.save_dot(dot_path)
        doc.img("digraph", doc.get_file_path(fname + ".png"))
