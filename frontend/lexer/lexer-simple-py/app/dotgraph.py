import os

import config

class DotGraph:

    def __init__(self, directed):
        self.directed = directed
        self.vertices = []
        self.edges = []

    def add_vertex(self, name):
        v = len(self.vertices)
        self.vertices.append(name)
        return v

    def add_edge(self, u, v):
        self.edges.append([u,v])

    def save_dot(self, dot_file):
        with open(dot_file, 'w') as f:
            f.write('digraph G{\n')

            for (v, name) in enumerate(self.vertices):
                f.write('  {} [ label="{}" ];\n'.format(v, name))

            for e in self.edges:
                f.write('  {} -> {}\n'.format(e[0], e[1]))

            f.write('}\n')



    def logia_doc_write(self, doc):
        if config.HIDE_DOT:
            doc.write("Graph: {} vertices\n".format(len(self.vertices)))
            return
        
        fname = doc.gen_file_name()
        dot_path = os.path.join(doc.out_dir(), fname + ".dot")
        self.save_dot(dot_path)
        doc.img("digraph", doc.get_file_path(fname + ".png"))
