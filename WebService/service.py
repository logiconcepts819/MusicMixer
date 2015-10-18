from flask import Flask, request, jsonify, Response
from gevent.wsgi import WSGIServer
import os, subprocess
from threading import Thread, Lock
from os import walk

app = Flask(__name__)

ON_POSIX = 'posix' in os.sys.builtin_module_names

lock = Lock()
lastSeenStatus = 'Ready'

def enqueue_output(out):
    while True:
        for line in iter(out.readline, b''):
            lock.acquire()
            lastSeenStatus = line
            lock.release()
    #out.close()

p = subprocess.Popen(['../mixing-pipe'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, bufsize=1, close_fds=ON_POSIX)
t = Thread(target=enqueue_output, args=(p.stdout))
t.daemon = True # thread dies with the program
t.start() 

musicPath = '/home/ron/Music'

@app.route('/v1/mixer/getSongs', methods=['GET'])
def getSongs():
    f = []
    for (dirpath, dirnames, filenames) in walk(musicPath):
        f.extend(filenames)
        break
    return jsonify({'message': sorted(f)})

@app.route('/v1/mixer/addSong', methods=['POST'])
def addSong():
    if request.form.get('filename'):
        filename = request.form.get('filename')
	if len(filename) > 0:
            p.stdin.write(filename + '\n')
            p.stdin.flush()
	return jsonify({'message': 'OK'})
    return jsonify({'message': 'No filename parameter'}), 400

@app.route('/v1/mixer/status', methods=['GET'])
def getStatus():
    lock.acquire()
    status = lastSeenStatus
    lock.release()
    return jsonify({'message': status})

@app.route('/', methods=['GET'])
def index():
    return app.send_static_file('index.html')

@app.route('/music-wallpaper22.jpg', methods=['GET'])
def musicWallpaper():
    return app.send_static_file('music-wallpaper22.jpg')

if __name__ == '__main__':
    print 'Mixing Web Service'
    print 'A vision by Ron Wright'
    http_server = WSGIServer(('', 5000), app)
    http_server.serve_forever()
