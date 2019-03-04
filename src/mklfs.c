/*
 ============================================================================
 Name        : mklfs.c
 Author      : Pratik Bhandari
 Version     : 1.0
 Description : The mklfs tool for LFS Project
 ============================================================================

 **  Usage  *****************************************************************

  mklfs [options] file
  where file is the name of the virtual flash file to create
	-b size, --block=size
        Size of a block, in sectors. The default is 2 (1KB).

	-l size, --segment=size
        Segment size, in blocks. The default is 32.

    -s sectors, --sectors=sectors
        Size of the flash, in sectors.  The default is 1024.

    -w limit, --wearlimit=limit
        Wear limit for erase blocks. The default is 1000.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <stdbool.h>
#include "flash.h"
#include <string.h>

static struct option long_options[] = {
    {"block", required_argument, NULL, 'b'},
    {"segment", required_argument, NULL, 'l'},
    {"segments", required_argument, NULL, 's'},
    {"wearlimit", required_argument, NULL, 'w'},
    {NULL, 0, NULL, 0}
};

bool lfs_create(long sector,
                long segment_size,
                int block_size,
                long wear_limit,
                char* file_name
                ) {

    // First delete the file if it already exists
    if( access( file_name, F_OK ) != -1 ) {
        if( remove( file_name ) != 0 )
            fprintf(stdout, "Error deleting file\n");
    }

    // Create the flash file
    int return_code = Flash_Create(file_name, wear_limit, sector/block_size)

    // If return code is not 0, something has gone wrong in creating the flash file
    if (return_code != 0) {
        fprintf(stderr, "Failure! Exiting now...\n")
        exit(EXIT_FAILURE)
    }
}

int main(int argc, char** argv)
{

    // Initializing the default values for the parameters
	long sector = 1024;
	long segment_size = 32;
	int block_size = 2;
    long wear_limit = 1000;
    char* file_name;

    // Get the flash file name
    file_name = argv[argc-1];

    int ch;
    int option_index = 0;

    // Figure out which flags have been explicitly stated
    while ((ch = getopt_long(argc, argv, "b:l:s:w:", long_options, &option_index)) != -1)
    {
        switch (ch) {
            
            // TODO: Handle case 0 as well??
            case 'b':
            // TODO: Do we do checks on individual flags to see if value exceeds 65536?
                block_size = atol(optarg);
                break;

            case 'l':
                segment_size = atol(optarg);
                break;
           
            case 's':
                sector = atol(optarg);
                break;
            
            case 'w':
                wear_limit = atol(optarg);
                break;
           
            case '?':
                break;

            default:
                printf("Invalid argument\n");
                abort ();
                break;         
        }
    }

    // Create the LFS file system
    bool lfs_create(sector, segment_size, block_size, wear_limit);
    
}
