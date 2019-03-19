// #include <fuse.h>
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
#include "log.h"

int main(int argc, char *argv[])
{
    // static long cache = 8;
    static long interval = 1000;
    static long start = 4;
    static long stop = 8;

    inputState state = inputState();
    state.lfsFile = "flash_test";
    state.interval = interval;
    // memcpy(state->lfsFile, flash_file, strlen(flash_file));
    // state->cacheSize = cache;
    // state->startCleaner = start;
    // state->stopCleaner = stop;
    // state->interval = interval;

    Directory directory = Directory((char*)state.lfsFile.c_str());

    directory.Initialize();
    //const char *path1 = "/users";
    //directory.makeDirectory(path1, S_ISUID);

    //directory.file.log.Flush();
    
    //test for read dir
    // char buffer[sizeof(Inode) + 1];
    // const char *path = "/users";
    // directory.Read(path, buffer, sizeof(Inode) + 1, 0, NULL);

    //test for create and open file.
    const char *path3 = "/users/foo";
    //directory.file.fileCreate(path3, S_ISUID, NULL);
    // Parse path to find inode
    Inode inode;
    int error = directory.file.ReadPath(path3, &inode);
    //char buffer[6] = "hel";
    //directory.file.fileWrite(&inode, 0, 4, buffer);

    char readBuffer[11];
    directory.file.fileRead(&inode, 0, 11, readBuffer);
    std::cout <<"done";







    // /* Prepare FUSE args */

    // char 	**nargv = NULL;
    // int     nargc;
    // nargc = 5;

    // nargv = (char **) malloc(nargc * sizeof(char*));
    // nargv[0] = argv[0];
    // nargv[1] = "-f";
    // nargv[2] = "-s";
    // nargv[3] = "-d";
    // nargv[4] = mount_point;
   
    /* start up FUSE */

    // return fuse_main(nargc, nargv, &file_oper, state);
}

