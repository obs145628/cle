import os
import subprocess
import sys

GIT_ROOT = subprocess.check_output(
    ['git', 'rev-parse', '--show-toplevel']).decode('ascii').strip()

LOGIA_MOD_PATH = os.path.join(GIT_ROOT, 'extern/logia/libpython')
sys.path.append(LOGIA_MOD_PATH)

HIDE_DOT=os.getenv("LOGIA_HIDE_DOT", "0") == "1"
