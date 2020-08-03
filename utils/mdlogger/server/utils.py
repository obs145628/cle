import os
import shutil
import time

ROOT_SRC_DIR = '../data'
ROOT_BUILD_DIR = '../html'
CSS_PATH = '../extra/styles.css'

def read_conf(path):
    res = dict()
    with open(path, 'r') as ifs:
        for l in ifs:
            data = l.strip().split('=', 1)
            res[data[0].strip()] = data[1].strip()
    return res

def md2html(md_file, html_file, title='md'):
    import markdown
    time_beg = time.time()

    CODE_FORMAT = '''
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8" />
<link rel="stylesheet" href="styles.css" />
<title>{}</title>
</head>
<body>{}</body>
<html>
'''
    
    with open(md_file, 'r', encoding='utf-8') as ifs:
        data = ifs.read()
    html = markdown.markdown(data, extensions=['codehilite', 'fenced_code'])
    with open(html_file, 'w', encoding='utf-8', errors='xmlcharrefreplace') as ofs:
        ofs.write(CODE_FORMAT.format(title, html))


    shutil.copyfile(CSS_PATH, os.path.join(os.path.dirname(html_file), "styles.css"))

    time_dur = time.time() - time_beg
    print('MD2HTML: {} => {} ({:.4f}s)'.format(md_file, html_file, time_dur))


def dot2png(dot_path, png_path):
    import subprocess
    time_beg = time.time()
    
    res = subprocess.run(['dot', '-Tpng', dot_path, '-o', png_path])
    if res.returncode != 0:
        raise Exception('dot command failed')

    time_dur = time.time() - time_beg
    print('dot -Tpng: {} => {} ({:.4f}s)'.format(dot_path, png_path, time_dur))
    
