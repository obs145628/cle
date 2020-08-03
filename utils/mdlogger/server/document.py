import os

import utils

OUT_DIR = './out'
CONF_PATH = './config.txt'

class Document:

    def __init__(self, doc_dir):
        self.src_path = os.path.join(utils.ROOT_SRC_DIR, doc_dir)
        self.build_path = os.path.join(utils.ROOT_BUILD_DIR, doc_dir)
        self.conf = None
        self.build_st = None

    def is_build(self):
        if self.build_st is None:
            self.build_st =  os.path.isdir(self.build_path)
        return self.build_st

    def get_conf(self):
        if self.conf is None:
            self.conf = utils.read_conf(os.path.join(self.src_path, CONF_PATH))
        return self.conf

    def get_order(self):
        return int(self.get_conf()['order'])

    def get_name(self):
        return self.get_conf()['name']

    def get_html_index_path(self):
        return os.path.join(self.build_path, 'index.html')

    def build_dot(self, dot_name):
        dot_path = os.path.join(self.src_path, dot_name)
        png_path = os.path.join(self.build_path, os.path.splitext(dot_name)[0] + '.png')
        utils.dot2png(dot_path, png_path)
        
    def build(self):
        if self.is_build():
            return

        os.makedirs(self.build_path, exist_ok=True)
        utils.md2html(os.path.join(self.src_path, 'main.md'), self.get_html_index_path(), title=self.get_name())

        for f in os.listdir(self.src_path):
            if f.endswith('.dot'):
                self.build_dot(f)
