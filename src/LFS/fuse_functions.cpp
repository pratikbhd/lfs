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
#include "fuse_functions.hpp"
#include"directory.hpp"

Directory *directory;

void* c_Initialize(struct fuse_conn_info *conn) {
    directory = new Directory(((inputState*)fuse_get_context()->private_data)->lfsFile);
    return directory->Initialize(conn);
}

int c_fileCreate(const char *name, mode_t mode, struct fuse_file_info *fi) {

    return directory->file.fileCreate(name, mode, fi);

}

int c_fileOpen(const char *name, struct fuse_file_info *fi) {

    return directory->file.fileOpen(name, fi);

}

int c_directoryRead(const char *path, char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi) {

    return directory->directoryRead(path, buf, length, offset, fi);

}

int c_directoryWrite(const char *path, const char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi) {

    return directory->directoryWrite(path, buf, length, offset, fi);

}

int c_fileGetattr(const char *path, struct stat *stbuf) {

    return directory->file.fileGetattr(path, stbuf);

}

int c_directoryReaddir(const char *path, void *buf, fuse_fill_dir_t filler, 
			off_t offset, struct fuse_file_info *fi) {

    return directory->directoryReaddir(path, buf, filler, offset, fi);

}

int c_makeDirectory(const char *path, mode_t mode) {

    return directory->makeDirectory(path, mode);

}

int c_Statfs(const char* path, struct statvfs *stbuf){
    return directory->Statfs(path, stbuf);
}

int c_File_Release(const char *path, struct fuse_file_info *fi) {
    // TODO
    std::cout <<"LFS: File_Release called";
    return 0;
}

void c_Destroy(void *data) {
    delete directory;
	std::cout << "LFS: Destroy called";
    return;
}

int c_Opendir(const char *name, struct fuse_file_info *finfo) {
	std::cout << "LFS: Open Dir called";
    return 0;
}

int c_Flush(const char *path, struct fuse_file_info *fi) {
    std::cout << "LFS: Flush called";
    return 0;
}

int c_Truncate(const char *path, off_t size) {
	std::cout << "LFS: Truncate called";
	return 0;
}

int c_HardLink(const char *to, const char *from) {
    std::cout << "LFS: Hardlink called";
	return 0;
}

int c_SymLink(const char *file, const char *sym) {
    std::cout << "LFS: Symlink called";
    return 0;
}

int c_ReadLink(const char *path, char *buf, size_t bufsize){
    std::cout << "LFS: Readlink called";
    return 0;
}

int c_Unlink(const char *path){
    std::cout << "LFS: Unlink called";
    return 0;
}

int c_Rmdir(const char *path) {
    std::cout << "LFS: Remove Directory called";
    return 0;
}

int c_Rename(const char *from, const char *to){
    std::cout << "LFS: Rename called";
    return 0;
}

int c_access(const char* path, int mask){
    std::cout <<"LFS: access called";
    return 0;
}