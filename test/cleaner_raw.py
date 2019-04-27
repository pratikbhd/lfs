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

    for index in range(50):
            print("ITERATION : " + str(index))
            path = os.path.join(options.mount, "foo")
            fd = open(path, "w")
            fd.write("hello")
            fd.close()
            fd = open(path, "r")
            contents = fd.read()
            fd.close()
            fd = open(path, "w")
            fd.write("hello")
            fd.write("goodbye")
            fd.close()
            fd = open(path, "r")
            contents = fd.read()
            fd.close()
            # fd = open(path, "w")
            # expected = "a" * 3 * 1024
            # fd.write(expected)
            # fd.close()
            # fd = open(path, "r")
            # contents = fd.read()
            # fd.close()
            path1 = os.path.join(options.mount, "foo"+str(index))
            fd = open(path1, "w")
            fd.write("hello")
            fd.close()
            path2 = os.path.join(options.mount, "bar"+str(index))

            if index == 38:
                print("reached 30th iteration!!")

            fd = open(path2, "w")
            fd.write("goodbye")
            fd.close()
            fd = open(path1, "r")
            contents1 = fd.read()
            fd.close()
            fd = open(path2, "r")
            contents2 = fd.read()
            fd.close()
            print(contents1)
            print(contents2)


if __name__ == '__main__': 
    main(sys.argv)
