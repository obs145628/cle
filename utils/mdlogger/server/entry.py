import os
from datetime import datetime

import utils
from document import Document

CONF_PATH = './config.txt'
INDEX_MD_PATH = './index.md'

class Entry:

    def __init__(self, dir_name):
        self.src_path = os.path.join(utils.ROOT_SRC_DIR, dir_name)
        self.build_path = os.path.join(utils.ROOT_BUILD_DIR, dir_name)
        self.conf = None
        
        self.docs = []
        for f in os.listdir(self.src_path):
            if os.path.isdir(os.path.join(self.src_path, f)):
                self.docs.append(Document(os.path.join(os.path.basename(self.src_path), f)))

        self.docs = sorted(self.docs,  key = lambda x : x.get_order())

    def is_build(self): 
        return os.path.isdir(self.build_path)

    def get_conf(self):
        if self.conf is None:
            self.conf = utils.read_conf(os.path.join(self.src_path, CONF_PATH))
        return self.conf

    def get_time(self):
        return int(self.get_conf()['time'])

    def get_datetime(self):
        return datetime.fromtimestamp(int(self.get_time()/1000))

    def get_cmd(self):
        return self.get_conf()['cmd']

    def get_cmd_binary(self):
        return self.get_cmd().split()[0]

    def gen_index_html(self):
        md_path = os.path.join(self.src_path, INDEX_MD_PATH)
        md_data = ''
        for doc in self.docs:
            doc_path = os.path.relpath(doc.get_html_index_path(),
                                       os.path.dirname(self.get_html_index_path()))
            md_data += '- [{}]({})\n'.format(doc.get_name(), doc_path)

        with open(md_path, 'w') as f:
            f.write(md_data)

        utils.md2html(md_path, self.get_html_index_path(), title=os.path.basename(self.get_cmd_binary()))
        os.remove(md_path)
        

    def get_html_index_path(self):
        return os.path.join(self.build_path, 'index.html')

    def build(self):
        if self.is_build():
            return

        for d in self.docs:
            if not d.is_build():
                d.build()
                Entry.g_entry_updated = True

        os.makedirs(self.build_path, exist_ok=True)
        self.gen_index_html()

    @staticmethod
    def list():
        if Entry.g_entries is None:
            data_dir = utils.ROOT_SRC_DIR
            res = []
            for f in os.listdir(data_dir):
                if os.path.isdir(os.path.join(data_dir, f)):
                    res.append(Entry(f))

            res = sorted(res, reverse=True, key = lambda x : x.get_time())
            Entry.g_entries = res
        return Entry.g_entries

    @staticmethod
    def update_root():
        if not Entry.g_entry_updated:
            return

        md_path = os.path.join(utils.ROOT_SRC_DIR, INDEX_MD_PATH)
        html_path = os.path.join(utils.ROOT_BUILD_DIR, 'index.html')
        md_data = ''
        for e in Entry.list():
            dt = e.get_datetime()
            dt = dt.strftime("%Y-%m-%d %H:%M")
            doc_path = os.path.relpath(e.get_html_index_path(),
                                       os.path.dirname(html_path))
            md_data += '- [{}]({}) ({})\n'.format(os.path.basename(e.get_cmd_binary()), doc_path, dt)
        
        with open(md_path, 'w') as f:
            f.write(md_data)
        utils.md2html(md_path, html_path, title='MDLogger')
        os.remove(md_path)

    @staticmethod
    def update_all():
        for e in Entry.list():
            e.build()
        Entry.update_root()
        Entry.g_entries = None
        Entry.g_entry_updated = False

    g_entries = None
    g_entry_updated = False
