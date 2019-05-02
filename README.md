# Log Structured File System in C++

This repository holds an implementation of a log structured file system that is mounted on a flash device and uses the FUSE user level file system interface to intercept file system calls to the mount point in the linux kernel to the mounted flash device.

### Build

We tested this in Ubuntu 18.04 using Fuse v2.9.9 and C++14.

To build lfs, mklfs and lfsck, we use g++ to compile and link our source code. Run the following commands to create the three binaries from the root directory of the github repository.

```
LFS:
g++ -g -o lfs -I headers src/LFS/LFS.cpp src/LFS/fuse_functions.cpp src/dir/directory.cpp src/file/cleaner.cpp src/file/file.cpp src/file/file_private.cpp src/log/log.cpp src/log/log_helper.cpp src/log/segment.cpp src/log/superblock.cpp src/log/checkpoint.cpp src/flash/flash.c `pkg-config --libs --cflags fuse`

# utilities:

mklfs:
g++ -g -o mklfs -I headers src/flash/mklfs.cpp src/log/log.cpp src/log/log_helper.cpp src/log/segment.cpp src/log/superblock.cpp src/log/checkpoint.cpp src/flash/flash.c

lfsck: 
g++ -g -o lfsck -I headers src/flash/lfsck.cpp src/file/cleaner.cpp 
src/file/file.cpp src/file/file_private.cpp
src/log/log.cpp src/log/log_helper.cpp src/log/segment.cpp 
src/log/superblock.cpp src/log/checkpoint.cpp src/flash/flash.c
```

### Run lfstest.py

1. Create a mount point directory location for fuse: egs: /tmp/flash/
2. Run lfstest.py by running the following command from the root directory of this repository:

```
python test/lfstest.py -f flash_file -m /tmp/flash/
```

### Launch 

To run the LFS filesystem, run mklfs first by running the binary 

```
./mklfs <flash_file>
```

You can also pass in arguments to set the segment count, blocks per segment, sectors per block and wear limit.

Next run the following binary:

```
./lfs <flash_file> <mount_point_for_fuse> <checkpoint_interval>
```


### Introduction

This design document describes our implementation of the Log Structured File System (LFS) in an object oriented fashion using C++. The LFS is designed in a layered approach comprising of the log layer, the file layer and the directory layer, all mounted on top of an emulated flash device composed of sectors and blocks. The following sections describe briefly the functionality of each layer.

### Log Layer

The log layer encapsulated by a Log object directly manages the flash device and acts as an interface between the flash device and higher up layers of the LFS. The flash layer is made up of a predefined number of sectors, fixed size of which are collectively considered to form a block. The log layer treats a collection of blocks in the flash layer as a segment. Read-write to and from the flash layer is executed in units of segments. The log layer further supports accessing blocks within a segment by a log address comprising of a segment number and block number. 

The Log object holds the last segment to which data is written to in an in-memory class member Segment object called log\_end. The segment object encapsulates functionality at the segment level such as Load, Flush and Erase segment. The log end segment in the log object is flushed to cache once a checkpoint is hit or if write operations exceed the segment size or if the file layer requests for a new segment.

A fixed size read-only segment cache is used to serve read requests. A segment which is not the log end, if requested for Read is loaded into the segment cache. Segments in the segment cache are replaced using a round robin cache updation policy. Segments already loaded in the segment cache are kept in sync with updates for that segment in the flash in real time.

The log layer abstracts I\slash O to the flash object with overloaded functions Write(), Read(), Free() etc for use by the higher layers.

The log layer object is owned by the file layer object which is responsible for initializing at file system mount and destroying the log object and it's dependencies at unmount.

The current limitations of the log layer include, the checkpoint blocks are stored in predetermined locations. All other functionalities that the log layer is responsible for are complete.

### File Layer

The file layer sits on top of the log layer and is directly below the directory layer in our LFS implementation. This layer handles the file abstraction and is represented in the form of an inode. Our implementation of an inode is a structure which stores the file metadata i.e. the inum of the inode, file type and the file size. Along with this, the inode also stores four direct block pointers to the first four blocks of the file that it represents and one indirect block pointer to the block of direct pointers.

Currently the file layer contains functions fileRead and  fileWrite to read and write data of a certain length when the inode of a file is provided. Function fileOpen opens a file for reading. Only the existence of the file is checked in order to open the file. Similarly, fileCreate creates and open a file for file operation. The fileRead and fileWrite functions will be overloaded to FUSE for system calls. In addition to this, other file layer functions like fileRelease, fileTruncate, fileFree and fileDelete are also handled.

We have implemented support for Hardlinks, softlinks and basic file permissions.

### Directory Layer

The directory layer is the uppermost layer and implements the directory hierarchy in our LFS design. It communicates with the file layer wherein it calls various functions that are required to initialize the operations on FUSE. So the directory layer implements the structure of the file system. Our current implementation has functions to initialize the directory structure, make a new directory, read from and write to a file (these are just wrappers to the file read and write functions).

The inodes for files and directories are stored in a special file called the ifile. These are the only places where inodes are stored and they too have inums similar to the inodes. We have designated a specific inum value of 1 for ifiles while the inode of the root directory will have an inode of 2.
In order to adhere to the top-down approach of our LFS implementation, the functions to create a new inode for a file/directory is defined in the file layer. When a new inode is needed, the function CreateInode first checks if there is an empty inode in the ifile. If no empty inodes are found, a new inode is added to the end of the ifile.

Directories are stored as a file as well. They are just an array of (name, inum) pairs.

### Fuse Functions

We support the following FUSE functions:

```
    .getattr = c_fileGetattr,
    .readlink = c_ReadLink,
    .mkdir = c_makeDirectory,
    .unlink = c_Unlink,
    .rmdir = c_Rmdir,
    .symlink = c_SymLink,
    .rename = c_Rename,
    .link = c_HardLink,
    .chmod = c_chmod,
    .chown = c_Chown,
    .truncate = c_Truncate,
    .open = c_fileOpen,
    .read = c_directoryRead,
    .write = c_directoryWrite,
    .statfs = c_Statfs,
    .readdir = c_directoryReaddir,
    .init = c_Initialize,
    .destroy = c_Destroy,
    .access = c_access,
    .create = c_fileCreate
```

### Segment Cleaner

We have implemented a segment cleaner that uses a cost-benefit ratio to pick candidate segments, cleaning them by moving occupied blocks to the end of the log.

### Crash Recovery

Crash recovery based on checkpoints is implemented. The last updated checkpoint from the two checkpoints available is used to obtain the log end.

The project does not implement roll-forward recovery.

### Utilities

We implemented two utilities in C++, mklfs and lfsck which are described below.

##### mklfs

The mklfs is an initialization script that sets up the basic LFS filesystem data structures in the flash device on which our filesystem ran. We use the entire first segment to store the Superblock containing file system level metadata and the iFile which has the inode map. The superblock stores several file system level metadata helps in address calculation and buffer allocation within our file system layers. The Superblock is represented as a class object of the same name in the log layer and is owned by the log object.

We reserved the first block of every segment to store the segment summary table and for ease of implementation we allocated the checkpoint blocks to static locations in the first segment of the flash along with the superblock and iFile. We plan to enhance this functionality to write checkpoint to the log end in phase 2.

##### lksck

lfsck is a C++ utility that we wrote to help with debugging and resolving several issues we faced and proved valuable during development. The lfsck extracts relevant file system data structures from the flash using the log layer and dumps a json file representative of the objects present in the file system.

The lfsck also has a python component that parses the json to python objects and runs automated checks to verify the metadata saved in the filesystem data structures. We plan to extend checks run by lfsck by phase 2. We ran mostly basic checks during initial stages of log layer development, and have relied mostly on the raw json more during phase 1 because of better flexibility in designing our file system structures.

### Contributors
- Rahul Roy Mattam = 60% features
- Pratik Bhandari = 40% features
