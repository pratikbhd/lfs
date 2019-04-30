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
        
    path = os.path.join(options.mount, "foo")
    fd = open(path, "w")
    fd.write("hello")
    fd.close()
    fd = open(path, "a")
    fd.truncate(10)
    fd.close()
    fd = open(path, "r")
    contents = fd.read()
    fd.close()
    assert contents == "hello\x00\x00\x00\x00\x00"


if __name__ == '__main__': 
    main(sys.argv)
