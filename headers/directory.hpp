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
		Directory(char* lfsFile);
		File file;

		// Setting variables to be used for various directory layer operations
		int inodes_length; // This is the length of the current inode
		int inodes_used; // This is an array of 0s and 1s that keep track of the current inodes being used in the ifile

		/*
 		* Initialize the directory structure
 		*/
		// void* Initialize(struct fuse_conn_info*);
		void* Initialize();

		/**
		 * Reads <length> bytes into <buf> from the file pointed by <path> starting at <offset> in the file. This is just an interface
		 * to fileRead.
		 */
		int Read(const char *path, char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi);

		/**
		 * Writes <length> bytes from <buf> to the file pointed by <path> starting at <offset> in the file. This is just an interface
		 * to fileWrite.
		 */
		int Write(const char *path, const char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi);

		/**
		 * Creates a new directory with the path <*path>. The <mode> argument is ignored and all permissions are positive.
		 */
		int makeDirectory(const char *path, mode_t mode);

		/**
		 * Reads the contents of the directory pointed by <path> and places the names of all files in <buf>. The entries '.' and '..' are
		 * also placed in <buf>.
		 */
		// int ReadDirectory(const char *path, void *buf, fuse_fill_dir_t filler, 
				// off_t offset, struct fuse_file_info *fi);
};