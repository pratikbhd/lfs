/*
 ============================================================================
 Name        : mklfs.cpp
 Author      : Pratik Bhandari and Rahul Roy Mattam
 Version     : 2.0
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

//#include <stdio.h>
//#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
//#include <time.h>
//#include <stdbool.h>
#include "flash.h"
#include "segment.h"
#include "json.hpp"
#include <string>
#include <iostream>
using json = nlohmann::json;

static struct option long_options[] = {
    {"block", required_argument, NULL, 'b'},
    {"segment", required_argument, NULL, 'l'},
    {"segments", required_argument, NULL, 's'},
    {"wearlimit", required_argument, NULL, 'w'},
    {NULL, 0, NULL, 0}
};

void lfs_init(Flash flash_handle, SuperBlock sb){
    int er = Flash_Erase(flash_handle, 0, sb.blockCount);
    if (er)
        fprintf(stderr, "ERASE ZERO SECTOR FAIL!\n");

    char *buffer = (char*)std::malloc(sizeof(SuperBlock)+ 1);
    memset(buffer, 0, sizeof(SuperBlock)+ 1);
    std::memcpy(buffer, &sb, sizeof(SuperBlock)+1);
    int res = Flash_Write(flash_handle, LOG_SUPERBLOCK_OFFSET, 1, buffer);
    if (res)
        std::cout << "Error writing log info" << std::endl;

    std::free(buffer);
    // Segment s1("hello", 10);
    // json j;
    // j["size"] = s1.size;
    // j["data"] = s1.data;
}

bool lfs_create(long no_of_segments,
                long blocks_per_segment,
                long wear_limit,
                char* file_name
                ) {

    //return true;
    // First delete the file if it already exists
    if( access( file_name, F_OK ) != -1 ) {
        if( remove( file_name ) != 0 )
            std::cout << "Error deleting file\n";
    }

    // Create the flash file
    int return_code = Flash_Create(file_name, wear_limit, no_of_segments * blocks_per_segment);

    //printf("%d",return_code);

    // If return code is not 0, something has gone wrong in creating the flash file
    if (return_code != 0) {
        std::cout << "Failure! Exiting now...\n";
        exit(EXIT_FAILURE);
    }
}


bool lfs_write_test(Flash flash_handle, int sector, int offset){
    return true;
    int rc;
    std::string opt1 = "hello";
    rc = Flash_Write(flash_handle, sector, offset, &opt1);
    std::cout << "the write" << opt1 << std::endl;
}

bool lfs_read_test(Flash flash_handle, int sector, int offset){
    //return true;
    char input[2048];
    int rc = Flash_Read(flash_handle, sector, offset, &input);
    std::cout << "the read" << input << std::endl;
}

int main(int argc, char** argv)
{
    std::cout << "Starting MKLFS" << std::endl;
    // Initializing the default values for the parameters
	long no_of_segments = 1024;
	long blocks_per_segment = 32;
    long wear_limit = 1000;
    char* file_name;

    // Get the flash file name
    file_name = argv[argc-1];

    int ch;
    int option_index = 0;

    // Figure out which flags have been explicitly stated
    while ((ch = getopt_long(argc, argv, "b:l:w:", long_options, &option_index)) != -1)
    {
        switch (ch) {
            
            // TODO: Handle case 0 as well??
            case 'b':
            // TODO: Do we do checks on individual flags to see if value exceeds 65536?
                blocks_per_segment = atol(optarg);
                break;

            case 'l':
                no_of_segments = atol(optarg);
                break;
           
            // case 's':
            //     sector = atol(optarg);
            //     break;
            
            case 'w':
                wear_limit = atol(optarg);
                break;
           
            case '?':
                break;

            default:
                std::cout << "Invalid argument\n";
                abort ();
                break;         
        }
    }

    // Create the LFS file system
    lfs_create(no_of_segments, blocks_per_segment, wear_limit, file_name);

    unsigned int blocks;
    auto flash = Flash_Open(file_name, 0, &blocks);
    SuperBlock sb;
    sb.segmentCount = no_of_segments;
    sb.blockCount = blocks;

    lfs_init(flash, sb);
    Flash_Close(flash);
}
