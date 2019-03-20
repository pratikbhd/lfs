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
		int *inodes_used; // This is an array of 0s and 1s that keep track of the current inodes being used in the ifile

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

		int innerReadDir(char *, int *, char *, int *);

		int Statfs(const char *path, struct statvfs *stbuf);

};