#include <sys/types.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <errno.h>
#include "log.h"
#include "segment.h"
#include "file.hpp"

#define FILE_MAX_SIZE   BLOCK_SIZE * (4 + (lInfo->bytesPerBlock) / sizeof(log_addr))
#define INODES_MAX_NUM  FILE_MAX_SIZE / sizeof(Inode)
#define BLOCK_SIZE lInfo->bytesPerBlock

class Directory {
	public:
		File file;

		// Setting variables to be used for various directory layer operations
		int inodes_length; // This is the length of the current inode
		int inodes_used; // This is an array of 0s and 1s that keep track of the current inodes being used in the ifile
		/*
 		* Create and delete the directory structure
 		*/
		void *Initialize(struct fuse_conn_info*);

		/**
		 * Creates a new directory with the path <*path>. The <mode> argument is ignored and all permissions are positive.
		 */
		int makeDirectory(const char *path, mode_t mode);

		/**
		 * Reads <length> bytes into <buf> from the file pointed by <path> starting at <offset> in the file. This is just an interface
		 * to File_Read.
		 */
		int Directory_Read(const char *path, char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi);

		/**
		 * Writes <length> bytes from <buf> to the file pointed by <path> starting at <offset> in the file. This is just an interface
		 * to File_Write.
		 */
		int Directory_Write(const char *path, const char *buf, size_t length, off_t offset,
	   					struct fuse_file_info *fi);
		
		/**
		 * Reads the contents of the directory pointed by <path> and places the names of all files in <buf>. The entries '.' and '..' are
 		* also placed in <buf>.
 		*/
		int Directory_Readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
			off_t offset, struct fuse_file_info *fi);

		/**
 		* Stores the Inode associated with <path> in the memory pointed by <inode>. If <path> does not lead to a file, then
 		* an error code is returned and <inode> points to undefined data.
 		*/
		int ReadPath(const char *path, Inode *inode);

		/**
		 * Creates a new Inode, and sets all of its members to a null value except for inum. This Inode is appended to the ifile.
		 * The new Inode will be pointed to by the <inode> pointer argument.
		 */
		int Directory_NewInode(Inode *inode);
		
		int Directory_ToggleUsedInum(int inum);
		
		int Directory_InumIsUsed(int inum);
		
		int Directory_CountInodes();
		/**
		 * Sets every value of <ino> to null except for the inum, then writes <ino> to the log.
		 */
		int Directory_ResetInode(Inode *ino);

		// Gets the maximum file size
		unsigned int GetMaxFileSize();
		/**
		 * Returns a pointer to the inode with <inum>.
		 */
		Inode *Directory_GetInode(int inum);

		/**
 		* Given a buffer <buf> of <length> bytes populated by the internalReadDir function, stores the inum of the file 
 		* with the given <name> at the location pointed to by <inum>.
 		*
 		* Returns 0 if <inum> is found, otherwise -ENOENT.
 		*/
		int inodeByNameInDirBuf(const char *buf, int length, const char *name, int *inum);

		/**
 		* Copies all of <path> except for the last link into a fresh buffer pointed to by <*parentPath> with terminating null byte. 
 		* The contents following are stored in <*child>, stripped of all '/' characters. Space is allocated for both using malloc 
 		* and should be freed by the caller. The length of parentPath is returned upon success. Otherwise, a negative number indicates 
 		* an error
 		*/
		int SplitPathAtEnd(const char *path, char **parentPath, char **child);
}
