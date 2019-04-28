#!/usr/bin/env python2.7
import sys
import os
import shutil
import subprocess
import optparse
import unittest
import stat
import time

def main(args):
    global options

    usage = "Usage %prog [options]"
    parser = optparse.OptionParser(version="1.2", usage=usage)

    parser.add_option("-q", "--quiet",
                      action="store_const", const=0, dest="verbosity", default=1,
                      help="quiet output")

    parser.add_option("-v", "--verbose",
                      action="count", dest="verbosity",
                      help="increase verbosity")

    parser.add_option("-F", "--failfast",
                      action="store_true", default=False, dest="failfast",
                      help="stop on first failed test")

    parser.add_option("-m", "--mount",
                      action="store", default=None, dest="mount",
                      help="mount point")

    (options, args) = parser.parse_args(args)

    if options.mount is None:
        raise Exception("You must use -m to specify a mount point.")

    path = os.path.join(options.mount, "testDir")
    # fd = open(path, "w")
    # fd.write("hello")
    # fd.close()
    os.mkdir(path)
    os.rmdir(path)
    # linkpath = os.path.join(options.mount, "bar")
    # os.link(path, linkpath)
    # os.unlink(path)


if __name__ == '__main__': 
    main(sys.argv)
