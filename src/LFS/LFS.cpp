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

#include "directory.hpp"
#include "log.h"

int main(int argc, char *argv[])
{
    static char *flash_file = "flash_test";
    static long cache = 8;
    static long interval = 1;
    static long start = 4;
    static long stop = 8;

    inputState *state = new inputState;
    memcpy(state->lfsFile, flash_file, strlen(flash_file));
    state->cacheSize = cache;
    state->startCleaner = start;
    state->stopCleaner = stop;
    state->interval = interval;

    Directory directory = new Directory;

    directory.Initialize();

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

