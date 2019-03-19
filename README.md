# Log Structured File System in C++

This repository holds an implementation of a log structured file system that is mounted on a flash device and uses the FUSE user level file system interface to intercept file system calls to the mount point in the linux kernel to the mounted flash device.

### Build

To build lfs, mklfs and lfsck, we use g++ to compile and link our source code. Run the following commands to create the three binaries from the root directory of the github repository.

```
mklfs:
g++ -g -o mklfs -I headers src/flash/mklfs.cpp src/log/log.cpp 
src/log/log_private.cpp src/log/segment.cpp src/log/superblock.cpp 
src/log/checkpoint.cpp src/flash/flash.c

lfsck: 
g++ -g -o lfsck -I headers src/flash/lfsck.cpp 
src/log/log.cpp src/log/log_private.cpp src/log/segment.cpp 
src/log/superblock.cpp src/log/checkpoint.cpp src/flash/flash.c

LFS:
g++ -g -o LFS -I headers src/LFS/LFS.cpp src/dir/directory.cpp 
src/file/file.cpp src/log/log.cpp src/log/log_private.cpp 
src/log/segment.cpp src/log/superblock.cpp src/log/checkpoint.cpp 
src/flash/flash.c
\end{lstlisting}
```

### Launch 

To run the LFS filesystem, run mklfs first by running the binary 

```
./mklfs <flash_file>
```

You can also pass in arguments to set the segment count, blocks per segment, sectors per block and wear limit.

Next run the following binary:

```
./LFS <flash_file> <mount_point_for_fuse> <checkpoint_interval>
```


### Introduction

This design document describes our implementation of the Log Structured File System (LFS) in an object oriented fashion using C++. The LFS is designed in a layered approach comprising of the log layer, the file layer and the directory layer, all mounted on top of an emulated flash device composed of sectors and blocks. The following sections describe briefly the functionality of each layer.

### Log Layer

The log layer encapsulated by a Log object directly manages the flash device and acts as an interface between the flash device and higher up layers of the LFS. The flash layer is made up of a predefined number of sectors, fixed size of which are collectively considered to form a block. The log layer treats a collection of blocks in the flash layer as a segment. Read-write to and from the flash layer is executed in units of segments. The log layer further supports accessing blocks within a segment by a log address comprising of a segment number and block number. \par

The Log object holds the last segment to which data is written to in an in-memory class member Segment object called log\_end. The segment object encapsulates functionality at the segment level such as Load, Flush and Erase segment. The log end segment in the log object is flushed to cache once a checkpoint is hit or if write operations exceed the segment size or if the file layer requests for a new segment.\par

The log layer abstracts I\slash O to the flash object with overloaded functions Write(), Read(), UpdateInode() and GetSuperblock() for use by the higher layers.\par

The log layer object is owned by the file layer object which is responsible for initializing at file system mount and destroying the log object and it's dependencies at unmount.

The current limitations of the log layer include, the checkpoint blocks are stored in predetermined locations, read segments are not cached in the log layer and block wear is not handled by the log layer. All other functionalities that the log layer is responsible for are complete.

### File Layer

The file layer sits on top of the log layer and is directly below the directory layer in our LFS implementation. This layer handles the file abstraction and is represented in the form of an inode. Our implementation of an inode is a structure which stores the file metadata i.e. the inum of the inode, file type and the file size. Along with this, the inode also stores four direct block pointers to the first four blocks of the file that it represents and one indirect block pointer to the block of direct pointers.\par
Currently the file layer contains functions \texttt{fileRead} and  \texttt{fileWrite} to read and write data of a certain length when the inode of a file is provided. Function \texttt{fileOpen} opens a file for reading. Only the existence of the file is checked in order to open the file. Similarly, \texttt{fileCreate} creates and open a file for file operation. The \texttt{fileRead} and \texttt{fileWrite} functions will be overloaded to \textbf{FUSE} for system calls. In addition to this, other file layer functions like \texttt{fileRelease}, \texttt{fileTruncate}, \texttt{fileFree} and \texttt{fileDelete} are also yet to be handled.

### Directory Layer

The directory layer is the uppermost layer and implements the directory hierarchy in our LFS design. It communicates with the file layer wherein it calls various functions that are required to initialize the operations on \textbf{FUSE}. So the directory layer implements the structure of the file system. Our current implementation has functions to initialize the directory structure, make a new directory, read from and write to a file (these are just wrappers to the file read and write functions). For the first phase, we aimed for a simplified version of the directory layer.\par
The inodes for files and directories are stored in a special file called the \textit{ifile}. These are the only places where inodes are stored and they too have inums similar to the inodes. We have designated a specific inum value of 1 for ifiles while the inode of the root directory will have an inode of 2.\par
In order to adhere to the top-down approach of our LFS implementation, the functions to create a new inode for a file/directory is defined in the file layer. When a new inode is needed, the function \texttt{CreateInode} first checks if there is an empty inode in the ifile. If no empty inodes are found, a new inode is added to the end of the ifile.\par
Directories are stored as a file as well. They are just an array of (name, inum) pairs. The standard '.' and '..' are not yet supported and multiple links to the same file are also not supported yet. These will be implemented in the second phase. Also, hard links and symbolic links are not supported as well and will be handled in the second phase. \par

### Fuse Functions

At the moment, we are still in the process of implementing \textbf{FUSE} to our prototype LFS design and expect to create a complete end-to-end system working in the next few days. Upon successful integration, we will start with the following \textbf{FUSE} will be supported.\functions
```
static struct fuse_operations file_oper = {
.init = Initialize,
.destroy = Destroy,
.statfs = Stats,
.create = Create,
.open = fileOpen,
.read = Read,
.write = Write,
.truncate = Truncate,
.rename = Rename
.getattr = fileGetattr,
.mkdir = makeDirectory,s
.rmdir = Directory_Rmdir,
.readdir = Directory_Readdir,
.opendir = Opendir, };
```

In addition to this, support for other \textbf{FUSE} functions to get stats, unlink, read hardlinks and symlink, remove directory, etc 
### Utilities

We implemented two utilities in C++, mklfs and lfsck which are described below.

##### mklfs

The mklfs is an initialization script that sets up the basic LFS filesystem data structures in the flash device on which our filesystem ran. We use the entire first segment to store the Superblock containing file system level metadata and the iFile which has the inode map. The superblock stores several file system level metadata helps in address calculation and buffer allocation within our file system layers. The Superblock is represented as a class object of the same name in the log layer and is owned by the log object.

We reserved the first block of every segment to store the segment summary table and for ease of implementation we allocated the checkpoint blocks to static locations in the first segment of the flash along with the superblock and iFile. We plan to enhance this functionality to write checkpoint to the log end in phase 2.

##### lksck

lfsck is a C++ utility that we wrote to help with debugging and resolving several issues we faced and proved valuable during development. The lfsck extracts relevant file system data structures from the flash using the log layer and dumps a json file representative of the objects present in the file system.

The lfsck also has a python component that parses the json to python objects and runs automated checks to verify the metadata saved in the filesystem data structures. We plan to extend checks run by lfsck by phase 2. We ran mostly basic checks during initial stages of log layer development, and have relied mostly on the raw json more during phase 1 because of better flexibility in designing our file system structures.

### Contributors
- Rahul Roy Mattam
- Pratik Bhandari
