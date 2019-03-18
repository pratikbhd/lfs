#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <errno.h>
#include "lfs_enums.h"
#include "log.h"

class File {
    public:
    Log log;
    File(char* lfsFile); // Need to pass the flashfile
    int fileWrite(Inode *inode, int offset, int length, const void *buffer);
    int fileRead (Inode *inode, int offset, int length, void *buffer);
    // int File_Free (Inode *inode);
    // int File_Getattr(const char *path, struct stat *stbuf);

    /**
     * Creates, opens, and returns a handle to a new file. If <path> is longer than
    * NAME_MAX_LENGTH, returs the error code ENAMETOOLONG and does nothing.
    **/
    // int fileCreate(const char *name, mode_t mode, struct fuse_file_info *fi);
    
    // int File_Open(const char *name, struct fuse_file_info *fi);
    // int File_Release(const char *path, struct fuse_file_info *fi);
    // int File_Flush(const char *path, struct fuse_file_info *fi);
    // int File_Truncate(Inode *, off_t);
    // int File_Delete(Inode *ino);
    unsigned int GetMaxFileSize();

    /**
 	* Creates a new Inode, and sets all of its members to a null value except for inum. This Inode is appended to the ifile.
 	* The new Inode will be pointed to by the <inode> pointer argument.
 	*/
	// int NewInode(Inode *inode);
};