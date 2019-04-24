#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <errno.h>
#include "lfs_enums.h"
#include "log.h"

class File {
    private:
        unsigned int operation_count;
        unsigned int max_operations = 1000;
        //Get the iFile from the flash.
        void getiFile();
        //Get a log address object stored at the specified index in the Inode block pointers.
        log_address getLogAddress(Inode i, int index);
        //Get next available free block at the log end.
        log_address getNewLogEnd();
        //Update the block pointer of the Inode i at the given block pointer array index to a new log address.
        bool updateInode(Inode *i, int index, log_address address);
        //Write or update a file with Inode and contents in buffer starting at the specified blocknumber upto length of buffer in bytes. 
        //length cannot exceed the maximum permitted file size. returns false if write cannot be completed.
        bool write(Inode *in, unsigned int blockNumber, int length, const char* buffer);
        //Trigger a checkpoint to flash.
        void checkpoint();
    public:
    std::vector<bool> inodes_used; // This is an array of 0s and 1s that keep track of the current inodes being used in the ifile

    Log log;
    Inode iFile;

    File(char* lfsFile); // Need to pass the flashfile

    ~File(); //Force Flush the log layer to flash and close flash.

    /**
    * This function writes out 'length' bytes of information from the buffer and into the file starting at 'offset'
    * The file to be written to is specified by the 'inode'
    */
    int fileWrite(Inode *inode, int offset, int length, const void *buffer);

    /**
    * This function reads in 'length' bytes of information from the file specified by the 'inode' and stores them in the 
    * buffer. Reading starts from 'offset'
    */ 
    int fileRead (Inode *inode, int offset, int length, char *buffer);

    /**
    * This function opens a file for reading. Nothing much is done here.
    * Only checks if the file exists or not.
    */
    int fileOpen(const char *name, struct fuse_file_info *fi);

    /**
    * This function first creates and then opens a new file.
    * The name of the file cannot be larger than the maximum file length specified in enums (lfs_enums.h) fileLength.
    **/
    int fileCreate(const char *name, mode_t mode, struct fuse_file_info *fi);
    
    /**
    * Reads the 'path' string and checks if the path exists and leads to a file. 
    * If it does not, an error code is returned and the 'inode' points to undefined data.
    * If successful, it points to the inode of the directory specified by the 'path'
    * This is mainly used to check whether a directory already exists when making a new directory or checking if a new one was made
    */
    int ReadPath(const char *path, Inode *inode);

    /**
    * This function gets the maximum file size that is permitted for a file to have.
    * The formula for this is: bytesPerBlock * (4 + (bytesPerBlock/sizeof(log_address)))
    */
    unsigned int GetMaxFileSize();

    /**
    * This function creates a new inode. The members are initialized to have a null value. The inum is specified to 0 i.e. NO_FILE.
    * This inode is added to the ifile.
    */
    int CreateInode(Inode *inode);
    
    /**
    * This function adds the (name, inum) pair to the file representing the directory with inode 'dir'. 
    * TODO: Work more on this. Current implementation is buggy.
    */
    int NewEntry(Inode *dir, Inode *file, const char *fileName);

    /**
    * This function stores the inum of the file given by the 'name' which is located at the location pointed by 'inum'
    */
    int ReturnInodeFromBuffer(const char *buf, int length, const char *name, int *inum);

    /**
    * This function returns a pointer to the inode having 'inum' 
    */
    Inode ReturnInode(int inum);

    bool ToggleInumUsage(int inum);

    /**
    * This function fills in the contents of the structure 'stbuf' of the FUSE.
    */
    int fileGetattr(const char *path, struct stat *stbuf);

    int Truncate(Inode *inode, off_t size);

    //Flush all log layer objects from memory to the flash.
    void Flush();
};