About MusicMixer
================

Inspiration
-----------

Music is such a big part of our lives in that currently music software does not fully maximize the potential of music. We address the issue by creating our own playlist program for playing back music, which crossfades songs on beat.

What it does
------------

The software emulates a real DJ when playing back music, which is meant to be mostly electronic. The song consists of metadata alongside the file, which is manually generated at present time. When a song ends, then metadata associated with the next song is combined with the metadata of the current song to blend the two songs together, which results in natural transitions.

How I built it
--------------

We built it using C++ for the backend, along with Qt for the desktop application. We also used Python and HTML for the web interface.
Challenges I ran into

We ran into many issues. One big issue was porting the application from Linux, which was the platform it was built for, to Windows. We had to install various libraries in order to port the program. Another issue was making the program look elegant. We noticed that the picture was distorted, and after careful implementation of fixes, we were able to resolve the problem.

Accomplishments that I'm proud of
---------------------------------

The program functions, and it can run on Windows. Also, the program has a frontend, which makes it usable by any user.

What I learned
--------------

I learned that sometimes implementing simple functionality in a short time frame can take a very long time.

What's next for MusicMixer
--------------------------

We hope to implement automatic beat detection and individual song selection. We also hope to perhaps make an Android or iOS version in the future.

Building MusicMixer
===================

This project contains several components: a C++ backend, a Qt frontend, a web
server interface, and a Windows installer.

C++ backend prerequisites
-------------------------

* MinGW (for Windows only)
* G++ with C++11 support
* PortAudio (http://portaudio.com)
* FFmpeg/LibAV (http://ffmpeg.org or http://libav.org)
* Rubberband Library (http://breakfastquay.com/rubberband)

Qt frontend prerequisites
-------------------------

* All C++ backend prerequisites
* Qt version 4 or higher

Web server interface prerequisites
----------------------------------

* All C++ backend prerequisites
* Python 2.x
* Python Pip
* Python VirtualEnv

Installer prerequisites
-----------------------

* All C++ backend and Qt frontend prerequisites
* NSIS

Building the Qt frontend in Windows
-----------------------------------

The Qt frontend and the C++ backend compile together for convenience. The
code successfully compiles with MinGW-w64. To build the C++ backend in Windows,
execute

```
qmake mixing-app.win.pro
```

and then execute

```
make
```

This should successfully build mixing-app.exe in the same directory.

Building the Qt frontend in Linux
---------------------------------

The steps for building the frontend in Linux are the same as the steps for
building in Windows, except you execute

```
qmake mixing-app.pro
```

first, and after executing make, the built app should have the name mixing-app
(without the extension).

Building the Web Server Interface
---------------------------------

Currently, the web server interface only supports Linux. The web service
communicates with the backend via named pipes. To build the pipe application,
execute

```
make-mixing-pipe.sh
```

which should create a binary named mixing-pipe. Then, go to the WebService
directory. If you do not see a folder known as venv (for a virtual Python
environment), execute the following command:

```
virtualenv venv
```

Afterwards, create a virtual Python environment:

```
source venv/bin/activate
```

If you have not already, install extra prerequisites that are listed in the
requirements.txt file:

```
pip install -r requirements.txt
```

Now you're ready to start the web service:

```
gunicorn -c gunicorn_config.py service:app
```

The server will start at http://localhost:5000/.

Building the Windows installer
------------------------------

The installer bundles only the Qt frontend (which contains the C++ backend),
since the web server currently supports Linux only. After building the Qt
frontend, build the installer with NSIS by selecting the installer.nsi file.
