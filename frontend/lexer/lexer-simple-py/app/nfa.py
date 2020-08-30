import os

# Static NFA
# Can only add transitions
class NFA:

    def __init__(self, alpha, states_count, start_state, final_states):
        self.alpha = alpha
        self.sc = states_count
        # adjency matrix for transitions
        # each entry is a set of chars
        self.adj = [set() for _ in range(self.sc*self.sc)]
        self.start = start_state
        self.finals = final_states

    def edges(self, s0, s1):
        return self.adj[s0 * self.sc + s1]

    # transition from state s0 to s1 with char c
    # c can be None for spontaneaous transitions
    def add_edge(self, s0, s1, c):
        self.edges(s0, s1).add(c)

    def save_dot(self, out_path):

        ENTRY = '__entry'
        
        with open(out_path, 'w') as f:
            f.write('digraph G {\n')
            f.write("rankdir=LR;\n")
            f.write('size="8,5"\n')
            f.write('node [shape = point ]; {};\n'.format(ENTRY))

            # Define states
            for s in range(self.sc):
                if s in self.finals.keys():
                    lbl = '{}:{}'.format(s, self.finals[s])
                    f.write('node [shape = doublecircle]; ')
                else:
                    lbl = s
                    f.write('node [shape = circle]; ')
                f.write('{} [ label="{}" ];\n'.format(s, lbl))
                

            # Draw transition for entry state
            f.write('{} -> {};\n'.format(ENTRY, self.start))
                
            #draw other transitions
            for s0 in range(self.sc):
                for s1 in range(self.sc):
                    es = list(self.edges(s0, s1))
                    if len(es) == 0:
                        continue
                    has_esp = None in es
                    if has_esp:
                        es.remove(None)
                    es_text = self.alpha.range_repr(es)
                    if has_esp:
                        es_text = "<>" + es_text
                    f.write('{} -> {} [ label="{}" ];\n'.format(s0, s1, es_text))

            
                
            f.write('}\n')


    def logia_doc_write(self, doc):
        fname = doc.gen_file_name()
        dot_path = os.path.join(doc.out_dir(), fname + ".dot")
        self.save_dot(dot_path)
        doc.img("NFA", doc.get_file_path(fname + ".png"))
