#define FUSE_USE_VERSION 26

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
#include "fuse_functions.hpp"

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


static struct fuse_operations file_oper {
    .getattr = c_fileGetattr,
    .readlink = c_ReadLink,
    .getdir = NULL,
    .mknod = NULL,
    .mkdir = c_makeDirectory,
    .unlink = c_Unlink,
    .rmdir = c_Rmdir,
    .symlink = c_SymLink,
    .rename = c_Rename,
    .link = c_HardLink,
    .chmod = NULL,
    .chown = NULL,
    .truncate = c_Truncate,
    .utime = NULL,
    .open = c_fileOpen,
    .read = c_directoryRead,
    .write = c_directoryWrite,
    .statfs = c_Statfs,
    .flush = c_Flush,
    .release = c_File_Release,
    .fsync = NULL,
    .setxattr = NULL,
    .getxattr = NULL,
    .listxattr = NULL,
    .removexattr = NULL,
    .opendir = c_Opendir,
    .readdir = c_directoryReaddir,
    .releasedir = NULL,
    .fsyncdir = NULL,
    .init = c_Initialize,
    .destroy = c_Destroy,
    .access = c_access,
    .create = c_fileCreate,
    .ftruncate = NULL,
    .fgetattr = NULL,
    .lock = NULL,
    .utimens = NULL,
    .bmap = NULL,
    .flag_nullpath_ok = 1,
    .flag_nopath = 1,
    .flag_utime_omit_ok = 1,
    .flag_reserved = 29,
    .ioctl = NULL,
    .poll = NULL,
    .write_buf = NULL,
    .read_buf = NULL,
    .flock = NULL,
    .fallocate = NULL,
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