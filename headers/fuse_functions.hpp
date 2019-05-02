#pragma once
#ifndef FUSE_USE_VERSION 
#define FUSE_USE_VERSION 26
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include<iostream>


#ifdef __cplusplus
extern "C" {
#endif
//implemented functions
int c_fileGetattr(const char *path, struct stat *stbuf);
int c_Truncate(const char *path, off_t size);
int c_fileCreate(const char *name, mode_t mode, struct fuse_file_info *fi);
void* c_Initialize(struct fuse_conn_info *conn);
int c_fileOpen(const char *name, struct fuse_file_info *fi);
int c_directoryRead(const char *path, char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi);

int c_access(const char* path, int mask);
int c_makeDirectory(const char *path, mode_t mode);

int c_directoryWrite(const char *path, const char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi); 

int c_Unlink(const char *path);
int c_Flush(const char *path, struct fuse_file_info *fi);

int c_Opendir(const char *name, struct fuse_file_info *finfo);
void c_Destroy(void *data);

int c_directoryReaddir(const char *path, void *buf, fuse_fill_dir_t filler, 
			off_t offset, struct fuse_file_info *fi);
int c_Statfs(const char* path, struct statvfs *stbuf);
int c_Chmod(const char* path, mode_t mode);

//stub only functions
int c_File_Release(const char *path, struct fuse_file_info *fi);
int c_Opendir(const char *name, struct fuse_file_info *finfo);
int c_Flush(const char *path, struct fuse_file_info *fi);
int c_HardLink(const char *to, const char *from);
int c_SymLink(const char *file, const char *sym);
int c_ReadLink(const char *path, char *buf, size_t bufsize);
int c_Unlink(const char *path);
int c_Rmdir(const char *path);
int c_Chown(const char *path, uid_t uid, gid_t gid);
int c_Utimens(const char *path, const struct timespec ts[2]);
int c_Rename(const char *from, const char *to);


#ifdef __cplusplus
} // extern "C"
#endif