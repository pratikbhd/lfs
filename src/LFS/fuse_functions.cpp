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
#include "color.hpp"

Directory *directory;

void* c_Initialize(struct fuse_conn_info *conn) {
    std::cout <<"[LFS] c_Initialize called: implemented" << std::endl;
    directory = new Directory(((inputState*)fuse_get_context()->private_data));
    return directory->Initialize(conn);
}

int c_fileCreate(const char *name, mode_t mode, struct fuse_file_info *fi) {
    std::cout <<"[LFS] c_fileCreate called: implemented"<< std::endl;
    return directory->file.fileCreate(name, mode, fi);

}

int c_fileOpen(const char *name, struct fuse_file_info *fi) {
    std::cout <<"[LFS] c_fileOpen called: implemented"<< std::endl;
    return directory->file.fileOpen(name, fi);
}

int c_directoryRead(const char *path, char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi) {
    std::cout <<"[LFS] c_directoryRead called: implemented"<< std::endl;
    return directory->directoryRead(path, buf, length, offset, fi);

}

int c_directoryWrite(const char *path, const char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi) {
    std::cout <<"[LFS] c_directoryWrite called: implemented"<< std::endl;
    return directory->directoryWrite(path, buf, length, offset, fi);

}

int c_fileGetattr(const char *path, struct stat *stbuf) {
    std::cout <<"[LFS] c_fileGetattr called: implemented"<< std::endl;
    return directory->file.fileGetattr(path, stbuf);
}

int c_directoryReaddir(const char *path, void *buf, fuse_fill_dir_t filler, 
			off_t offset, struct fuse_file_info *fi) {
    std::cout <<"[LFS] c_directoryReaddir called: implemented"<< std::endl;
    return directory->directoryReaddir(path, buf, filler, offset, fi);
}

int c_makeDirectory(const char *path, mode_t mode) {
    std::cout <<"[LFS] mkdir called: implemented"<< std::endl;
    return directory->makeDirectory(path, mode);

}

int c_Statfs(const char* path, struct statvfs *stbuf){
    std::cout <<"[LFS] c_Statfs called: implemented"<< std::endl;
    return directory->Statfs(path, stbuf);
}

int c_Truncate(const char *path, off_t size) {
	std::cout << "[LFS] Truncate called: implemented"<< std::endl;
    return directory->Truncate(path, size);
}

void c_Destroy(void *data) {
    delete directory;
	std::cout << "[LFS] Destroy called: implemented"<< std::endl;
    return;
}

int c_access(const char* path, int mask){
    std::cout <<"[LFS] access called: implemented"<< std::endl;
    return directory->Exists(path);
}

int c_Chown(const char *path, uid_t uid, gid_t gid) {
    Color::Modifier red(Color::FG_RED);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << red <<"[LFS] Chown called: stub only"<< def <<std::endl;
    return 0;
}

int c_Utimens(const char *path, const struct timespec ts[2]) {
    Color::Modifier red(Color::FG_RED);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << red <<"[LFS] Utimens called: stub only"<< def <<std::endl;
    return 0;
}

int c_File_Release(const char *path, struct fuse_file_info *fi) {
    Color::Modifier red(Color::FG_RED);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << red <<"[LFS] File_Release called: stub only"<< def <<std::endl;
    return 0;
}

int c_Opendir(const char *name, struct fuse_file_info *finfo) {
    Color::Modifier red(Color::FG_RED);
    Color::Modifier def(Color::FG_DEFAULT);
	std::cout << red <<"[LFS] Open Dir called: stub only"<< def <<std::endl;
    return 0;
}

int c_Flush(const char *path, struct fuse_file_info *fi) {
    Color::Modifier red(Color::FG_RED);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << red <<"[LFS] Flush called: stub only"<< std::endl;
    return 0;
}

int c_HardLink(const char *to, const char *from) {
    Color::Modifier red(Color::FG_RED);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << red <<"[LFS] Hardlink called: stub only"<< def<< std::endl;
	return directory->createHardLink(to, from);
}

int c_SymLink(const char *to, const char *sym) {
    Color::Modifier red(Color::FG_RED);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << red <<"[LFS] Symlink called: stub only"<< def<< std::endl;
    return directory->createSymLink(to, sym);
}

int c_ReadLink(const char *path, char *buf, size_t bufsize){
    Color::Modifier red(Color::FG_RED);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << red <<"[LFS] Readlink called: stub only"<< def<< std::endl;
    return directory->readLink(path, buf, bufsize);
}

int c_Unlink(const char *path){
    Color::Modifier green(Color::FG_GREEN);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << green <<"[LFS] Unlink: Implemented"<< def<< std::endl;
    return directory->unlink(path);
}

int c_Rmdir(const char *path) {
    Color::Modifier green(Color::FG_GREEN);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << green <<"[LFS] Remove Directory: Implemented"<< def<< std::endl;
    return directory->removeDirectory(path);
}

int c_Rename(const char *from, const char *to){
    Color::Modifier red(Color::FG_RED);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << red <<"[LFS] Rename called: stub only"<< def<< std::endl;
    return directory->rename(from, to);

}