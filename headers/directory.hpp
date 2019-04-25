#pragma once
#include <sys/types.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <errno.h>
#include "log.h"
#include "segment.h"
#include "file.hpp"

class Directory {
	public:
		Directory(inputState* state);
		File file;

		// Setting variables to be used for various directory layer operations
		int inodes_length; // This is the length of the current inode

		/*
 		* Initializes the directory structure
 		*/
		void* Initialize(struct fuse_conn_info*);
		// void* Initialize();

		/**
		 * Just a wrapper for the fileRead function in the file layer. It reads in 'length' bytes of data into the buffer from the 'path'
		 * starting at the 'offset'
		 */
		int directoryRead(const char *path, char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi);

		/**
		 * Just a wrapper for the fileWrite function in the file layer. It writes out 'length' bytes of data from the buffer to the file at 'path',
		 * starting at the provided 'offset'
		 */
		int directoryWrite(const char *path, const char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi);

		/**
		 * Create a new directory from the provided 'path'. The 'mode' argument is used to set directory permissions. For now, it is not used
		 * TODO: Implement 'mode' for phase 2
		 */
		int makeDirectory(const char *path, mode_t mode);

		int directoryReaddir(const char *path, void *buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info *fi);

		/**
 		* Finds the first item found in the directory. inum will point to the inode number, name to the
		* file name, and newOffset to the point in directory where the next entry begins. name should be
 		* buffer of size at least NAME_MAX_LENGTH+1.
 		*/
		int innerReadDir(char *, int *, char *, int *);

		int CountInodes();

		int Truncate(const char *path, off_t size);

		//Check if a file or directory exists at this path or not.
		int Exists(const char * path);

		int Statfs(const char *path, struct statvfs *stbuf);

		/**
 		* Link functions
 		*/

		/**
		 * Creates a file with path <*from> which links to the same file as <*to>.
		 */
		int createHardLink(const char *to, const char *from);

		/**
		 * Creates a symbolic link at <*from> with contents <*to>
		 */
		int createSymLink (const char *to, const char *from);

		/**
 		* Opens the symbolic link <*path> and stores the path of the linked file in <*buf>.
 		*/
		int readLink(const char *path, char *buf, size_t bufsize);

		/**
 		* Removes <*name> from its directory, and decrements the hardlink count of the files inode. If the count is 0, then
 		* the file is deleted from the log using File_Delete.
 		*/
		int unlink(const char *name);

		/**
 		* Copies all of <path> except for the last link into a fresh buffer pointed to by <*parentPath> with terminating null byte.
 		* The contents following are stored in <*child>, stripped of all '/' characters. Space is allocated for both using malloc
 		* and should be freed by the caller. The length of parentPath is returned upon success. Otherwise, a negative number indicates
 		* an error
 		*/
		int splitPathAtEnd(const char *path, char **parentPath, char **child);

		/**
		* Delete functions
		*/

		/**
		* Removes the directory <path>. If this directory is not empty, removeDirectory fails and returns -ENOTEMPTY.
   		* If it is not a directory, removeDirectory fails and returns -ENOTDIR. Otherwise, the directory is deleted
		* and 0 is returned.
		*/
		int removeDirectory(const char * path);

		/**
 		* Removes the file named <name> associated with <ino> from the directory <dir>.
 		* The inodes are updated in the log.
 		*/
		int deleteEntry(Inode *dir, Inode *ino, const char *name);

		/**
 		* Renames the entry <from> with the name <to>
 		*/
		int rename(const char *from, const char *to);
	
};