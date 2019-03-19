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
    int fileRead (Inode *inode, int offset, int length, char *buffer);
    int fileOpen(const char *name, struct fuse_file_info *fi);

    /**
    * Creates, opens, and returns a handle to a new file. If <path> is longer than
    * NAME_MAX_LENGTH, returs the error code ENAMETOOLONG and does nothing.
    **/
    int fileCreate(const char *name, mode_t mode, struct fuse_file_info *fi);
    
    /**
    * Stores the Inode associated with <path> in the memory pointed by <inode>. If <path> does not lead to a file, then
    * an error code is returned and <inode> points to undefined data.
    */
    int ReadPath(const char *path, Inode *inode);

    unsigned int GetMaxFileSize();

    /**
    * Creates a new Inode, and sets all of its members to a null value except for inum. This Inode is appended to the ifile.
    * The new Inode will be pointed to by the <inode> pointer argument.
    */
    int CreateInode(Inode *inode);
    
    /**
    * Adds the pair <file->inum, *fileName> to the directory with Inode <*dir>. The entry is added in lexicographical order
    * with respect to <*fileName>.
    */
    int NewEntry(Inode *dir, Inode *file, const char *fileName);

    /**
    * Given a buffer <buf> of <length> bytes populated by the internalReadDir function, stores the inum of the file 
    * with the given <name> at the location pointed to by <inum>.
    *
    * Returns 0 if <inum> is found, otherwise -ENOENT.
    */
    int ReturnInodeFromBuffer(const char *buf, int length, const char *name, int *inum);

    /**
    * Returns a pointer to the inode with <inum>.
    */
    Inode *ReturnInode(int inum);
};