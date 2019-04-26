#!/usr/bin/env python2.7
import sys
import os
import shutil
import subprocess
import optparse
import unittest
import stat
import time

# NOTES
# These are the phase 1 LFS test cases. You must specify the flash device and mount point as
# arguments -- see the bottom of this file. The test cases will create the flash device and mount
# point so they should not exist already. These tests expect the following files to exist in the
# current working directory:
#       mklfs -- as per the handout
#       lfs -- as per the handout
#       umount -- (optional) this script allows you to unmount your lfs in a platform specific
#                 way. By default the tests assume Mac OSX. On a different platform create a 
#                 script that unmounts its argument. If anyone knows a better way to unmount
#                 from Python in a platform-specific way please let me know.

class TestBase(unittest.TestCase):

    def _mklfs(self, flash, bs = 2, ss = 32, ns = 1024, wear=1000):
        cmd = ['./mklfs', '-l', str(ss), '-s', str(ns), '-b', str(bs), '-w', str(wear), flash]
        subprocess.check_call(cmd)
        if not os.path.exists(flash):
            raise Exception("mklfs did not create flash device correctly")


    def _lfs(self, flash, mount, interval = 10, cache = 10):
        subprocess.Popen(['./lfs', '-i', str(interval), '-c', str(cache), flash, mount])
        time.sleep(1)

    def setUp(self):
        global options
        self._nuke()
        self._mklfs(options.flash)
        os.mkdir(options.mount)
        self._lfs(options.flash, options.mount)

    def _nuke(self):
        global options
        if os.path.exists("./umount"):
            cmd = ['./umount', options.mount]
        else:
            cmd = ['fusermount', '-uz', options.mount]
        subprocess.call(cmd)
        if os.path.exists(options.mount):
            os.rmdir(options.mount)

    def tearDown(self):
        self._nuke()

class TestBasic(TestBase):

    def test_00_cleaner(self):
        global options
        for index in range(20):
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

    parser.add_option("-f", "--flash",
                      action="store", default=None, dest="flash",
                      help="flash device")

    parser.add_option("-m", "--mount",
                      action="store", default=None, dest="mount",
                      help="mount point")

    (options, args) = parser.parse_args(args)

    if options.flash is None:
        raise Exception("You must use -f specify a flash device.")

    if options.mount is None:
        raise Exception("You must use -m to specify a mount point.")

    unittest.main(argv=args, verbosity=options.verbosity, failfast=options.failfast)

if __name__ == '__main__': 
    main(sys.argv)
