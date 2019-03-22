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
    std::cout <<"[LFS] c_Initialize called: implemented";
    directory = new Directory(((inputState*)fuse_get_context()->private_data)->lfsFile);
    return directory->Initialize(conn);
}

int c_fileCreate(const char *name, mode_t mode, struct fuse_file_info *fi) {
    std::cout <<"[LFS] c_fileCreate called: implemented";
    return directory->file.fileCreate(name, mode, fi);

}

int c_fileOpen(const char *name, struct fuse_file_info *fi) {
    std::cout <<"[LFS] c_fileOpen called: implemented";
    return directory->file.fileOpen(name, fi);

}

int c_directoryRead(const char *path, char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi) {
    std::cout <<"[LFS] c_directoryRead called: implemented";
    return directory->directoryRead(path, buf, length, offset, fi);

}

int c_directoryWrite(const char *path, const char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi) {
    std::cout <<"[LFS] c_directoryWrite called: implemented";
    return directory->directoryWrite(path, buf, length, offset, fi);

}

int c_fileGetattr(const char *path, struct stat *stbuf) {
    std::cout <<"[LFS] c_fileGetattr called: implemented";
    return directory->file.fileGetattr(path, stbuf);
}

int c_directoryReaddir(const char *path, void *buf, fuse_fill_dir_t filler, 
			off_t offset, struct fuse_file_info *fi) {
    std::cout <<"[LFS] c_directoryReaddir called: implemented";
    return directory->directoryReaddir(path, buf, filler, offset, fi);
}

int c_makeDirectory(const char *path, mode_t mode) {
    std::cout <<"[LFS] mkdir called: implemented";
    return directory->makeDirectory(path, mode);

}

int c_Statfs(const char* path, struct statvfs *stbuf){
    std::cout <<"[LFS] c_Statfs called: implemented";
    return directory->Statfs(path, stbuf);
}

int c_Truncate(const char *path, off_t size) {
	std::cout << "[LFS] Truncate called: implemented";
    return directory->Truncate(path, size);
}

void c_Destroy(void *data) {
    delete directory;
	std::cout << "[LFS] Destroy called: implemented";
    return;
}

int c_access(const char* path, int mask){
    std::cout <<"[LFS] access called: implemented";
    return directory->Exists(path);
}

int c_File_Release(const char *path, struct fuse_file_info *fi) {
    // TODO
    std::cout <<"[LFS] File_Release called: stub only";
    return 0;
}

int c_Opendir(const char *name, struct fuse_file_info *finfo) {
	std::cout << "[LFS] Open Dir called: stub only";
    return 0;
}

int c_Flush(const char *path, struct fuse_file_info *fi) {
    std::cout << "[LFS] Flush called: stub only";
    return 0;
}

int c_HardLink(const char *to, const char *from) {
    std::cout << "[LFS] Hardlink called: stub only";
	return 0;
}

int c_SymLink(const char *file, const char *sym) {
    std::cout << "[LFS] Symlink called: stub only";
    return 0;
}

int c_ReadLink(const char *path, char *buf, size_t bufsize){
    std::cout << "[LFS] Readlink called: stub only";
    return 0;
}

int c_Unlink(const char *path){
    std::cout << "[LFS] Unlink called: stub only";
    return 0;
}

int c_Rmdir(const char *path) {
    std::cout << "[LFS] Remove Directory called: stub only";
    return 0;
}

int c_Rename(const char *from, const char *to){
    std::cout << "[LFS] Rename called: stub only";
    return 0;
}