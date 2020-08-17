import os

def dot2png(dot_file, png_file):
    os.system('dot -Tpng {} -o {}'.format(dot_file, png_file))

def show_file(dot_file):
    if os.getenv('HIDE_DOT_GUI', '0') != '0':
        return
    
    png_file = '/tmp/gbx_cl_py_graphivz.png'
    dot2png(dot_file, png_file)
    os.system('feh {}'.format(png_file))

def show_obj(obj):
    dot_file = '/tmp/gbx_cl_py_graphivz.dot'
    obj.save_dot(dot_file)
    show_file(dot_file)
