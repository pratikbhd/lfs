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

#include "directory.hpp"
#include "file.hpp"
#include "log.h"

static int opt = 0;

// For getting long options
#define CACHE "cache"
#define INTERVAL "interval"
#define START "start"
#define STOP  "stop"

static struct option long_options[] = {
    {CACHE, 1, 0, 0},
    {INTERVAL, 1, 0, 0},
    {START, 1, 0, 0},
    {STOP, 1, 0, 0},
    {NULL, 0, NULL, 0}
};

static char *mount_point;
static char *flash_file ;
static long cache = 8;
static long interval = 1;
static long start = 4;
static long stop = 8;

static Directory directory = Directory(((inputState*)fuse_get_context()->private_data)->lfsFile);

void* c_Initialize(struct fuse_conn_info *conn) {

    return directory.Initialize(conn);

}

int c_fileCreate(const char *name, mode_t mode, struct fuse_file_info *fi) {

    return directory.file.fileCreate(name, mode, fi);

}

int c_fileOpen(const char *name, struct fuse_file_info *fi) {

    return directory.file.fileOpen(name, fi);

}

int c_directoryRead(const char *path, char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi) {

    return directory.directoryRead(path, buf, length, offset, fi);

}

int c_directoryWrite(const char *path, const char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi) {

    return directory.directoryWrite(path, buf, length, offset, fi);

}

int c_fileGetattr(const char *path, struct stat *stbuf) {

    return directory.file.fileGetattr(path, stbuf);

}

int c_directoryReaddir(const char *path, void *buf, fuse_fill_dir_t filler, 
			off_t offset, struct fuse_file_info *fi) {

    return directory.directoryReaddir(path, buf, filler, offset, fi);

}

int c_makeDirectory(const char *path, mode_t mode) {

    return directory.makeDirectory(path, mode);

}


void parse_args (int argc, char **argv)
{
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "s:i:c:C:",long_options, &option_index)) != -1) {
        switch (opt) {
            case 0:
                printf ("option %s", long_options[option_index].name);
                if (optarg) {
                    printf(" with value %s", optarg);
                    if(strcmp(long_options[option_index].name, CACHE) == 0) {
                        cache = atol(optarg);
                    } else if (strcmp(long_options[option_index].name, INTERVAL) == 0) {
                        interval = atol(optarg);
                    } else if (strcmp(long_options[option_index].name, START) == 0) {
                        start = atol(optarg);
                    } else if (strcmp(long_options[option_index].name, STOP) == 0) {
                        stop = atol(optarg);
                    } else {
                        printf("<<< invalid argument, exiting >>>\n");
                        exit(EXIT_FAILURE);
                    }
                }
                printf ("\n");
                break;

            case 's':
                printf("arg s with arg %s\n", optarg);
                cache = atol(optarg);
                break;
            
            case 'i':
                printf("arg i with arg %s\n", optarg);
                interval = atol(optarg);
                break;
            
            case 'c':
                printf("arg c with arg %s\n", optarg);
                start = atol(optarg);
                break;
            
            case 'C':
                printf("arg C with arg %s\n", optarg);
                stop = atol(optarg);
                break;

            default:
                printf("<<< invalid argument, exiting >>>\n");
                exit(EXIT_FAILURE);
                break;
        }
    }
}

static struct fuse_operations file_oper = {
    .init     = c_Initialize,
    .create   = c_fileCreate,
    .open     = c_fileOpen,
    .read     = c_directoryRead,
    .write    = c_directoryWrite,
    .getattr  = c_fileGetattr,
    .readdir  = c_directoryReaddir,
	.mkdir    = c_makeDirectory,
};

int main(int argc, char *argv[])
{
    mount_point = argv[argc-1];
    flash_file = argv[argc-2];
    parse_args(argc, argv);

    static long cache = 8;
    static long interval = 1000;
    static long start = 4;
    static long stop = 8;

    inputState state = inputState();
    state.lfsFile = flash_file;
    state.interval = interval;

    mount_point = "/tmp/fs/";
    flash_file = "flash_test";

    std::cout << "lfs file: " << state.lfsFile << std::endl;
    std::cout << "interval: " << state.interval << std::endl;

    // Directory directory = Directory((char*)state.lfsFile.c_str());

    // directory.Initialize();
    // const char *path = "/FlashDir";
    // directory.makeDirectory(path, S_ISUID);

    // directory.file.log.Flush();
    
    // test for read dir
    // char buffer[sizeof(Inode) + 1];
    // const char *path = "/users";
    // directory.Read(path, buffer, sizeof(Inode) + 1, 0, NULL);

    // Preparing FUSE arguments 

    char 	**nargv = NULL;
    int     nargc;
    nargc = 5;

    nargv = (char **) malloc(nargc * sizeof(char*));
    nargv[0] = argv[0];
    nargv[1] = "-f";
    nargv[2] = "-s";
    nargv[3] = "-d";
    nargv[4] = mount_point;

    return fuse_main(nargc, nargv, &file_oper, &state);
}