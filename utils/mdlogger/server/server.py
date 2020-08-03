from flask import Flask, send_from_directory

import utils
from entry import Entry

def serve_static(path):
    return send_from_directory(utils.ROOT_BUILD_DIR, path)


app = Flask(__name__)

@app.route('/')
def send_root():
    Entry.update_all()
    return serve_static('index.html')

@app.route('/<path:path>')
def send_file(path):
    return serve_static(path)



if __name__ == "__main__":
    Entry.update_all()
    app.run(port=8890)
