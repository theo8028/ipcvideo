# setup.py

from distutils.core import setup, Extension

setup(  name = "ipcvideo",
        version = "0.1",
        description = "An open-source ipc(Interprocess Communication) video library.",
        author = "theo",
        author_email = "theo8028@gmail.com",
        url = "https://ipcvideo.blogspot.com/",
        ext_modules = [Extension("ipcvideo", ["ipcvideo.cpp","ipcvideo_base.cpp"])]
        )