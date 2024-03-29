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
#include "lfs_enums.h"
#include "segment.h"
#include "log.h"
#include "json.hpp"
#include <string>
#include <iostream>
#include <ctime>
using json = nlohmann::json;

static struct option long_options[] = {
    {"block", required_argument, NULL, 'b'},
    {"segment", required_argument, NULL, 'l'},
    {"segments", required_argument, NULL, 's'},
    {"wearlimit", required_argument, NULL, 'w'},
    {NULL, 0, NULL, 0}
};

void lfs_init_superblock(Log *l){
    int er = Flash_Erase((*l).flash, 0, (*l).super_block.blockCount);
    if (er)
        fprintf(stderr, "[MKLFS] ERASE ZERO SECTOR FAIL!\n");

    char *buffer = (char*)std::malloc(sizeof(SuperBlock)+ 1);
    memset(buffer, 0, sizeof(SuperBlock)+ 1);
    std::memcpy(buffer, &(*l).super_block, sizeof(SuperBlock)+1);
    std::cout << "[MKLFS] writing superblock at offset: "<< LOG_SUPERBLOCK_OFFSET << std::endl;
    int res = Flash_Write((*l).flash, LOG_SUPERBLOCK_OFFSET, 1, buffer);
    if (res)
        std::cout << "[MKLFS] Error writing superblock info" << std::endl;

    std::free(buffer);
}

void lfs_init_segmentsummary(Log *l){
    std::cout << "[MKLFS] writing segment summary block" << std::endl;
    for(unsigned int i=1; i < (*l).super_block.segmentCount; i++) {
        block_usage b[(*l).super_block.blocksPerSegment];
        unsigned int j = 0;
        while (j < (sizeof(b)/(*l).super_block.bytesPerBlock) + 1) {
            b[j].use = static_cast<char>(usage::INUSE);
            b[j].inum = static_cast<unsigned int>(reserved_inum::NOINUM);
            b[j].age = std::time(nullptr);
            j++;
        }
        for(; j < (*l).super_block.blocksPerSegment; j++) {
            b[j].use = static_cast<char>(usage::FREE);
            b[j].inum = static_cast<unsigned int>(reserved_inum::NOINUM);
            b[j].age = std::time(nullptr);
        }
        unsigned int offset = i*(((*l).super_block.sectorsPerBlock)*((*l).super_block.blocksPerSegment));
        int res = Flash_Write((*l).flash, offset, (*l).super_block.sectorsPerBlock, b);
        if (res) {
            std::cout << "[MKLFS] MKLFS summary map failed: " << offset << std::endl;
        }
    }
}

void lfs_init_checkpoint(Log *l){
    Checkpoint check_point((*l).GetLogAddress(0, 0), 0);
    char *buffer = (char*)std::malloc(sizeof(Checkpoint)+ 1);

    //Write First Checkpoint
    memset(buffer, 0, sizeof(Checkpoint)+ 1);
    memcpy(buffer, &check_point, sizeof(Checkpoint)+1);
    std::cout << "[MKLFS] writing checkpoint one at offset: "<< LOG_CP1_OFFSET << std::endl;
    int res = Flash_Write((*l).flash, LOG_CP1_OFFSET, 1, buffer);
    if (res) {
        std::cout << "[MKLFS] MKLFS WRITE FAIL:" << res << std::endl;
    }
    
    //Write Second Checkpoint
    memset(buffer, 0, sizeof(Checkpoint)+ 1);
    memcpy(buffer, &check_point, sizeof(Checkpoint)+1);
    std::cout << "[MKLFS] writing checkpoint two at offset: "<< LOG_CP2_OFFSET << std::endl;
    res = Flash_Write((*l).flash, LOG_CP2_OFFSET, 1, buffer);
    if (res) {
        std::cout << "[MKLFS] MKLFS WRITE FAIL" << std::endl;
    }

    std::free(buffer);
}

void lfs_init_inodemap(Log *l) {
    /* Create iFile */
    Inode iFile = Inode();
    iFile.inum = static_cast<unsigned int>(reserved_inum::IFILE);
    iFile.fileSize = 0;
    iFile.fileType = static_cast<char>(fileTypes::IFILE);

    log_address address = {0, 0};
    iFile.block_pointers[0]= address;
    iFile.block_pointers[1]= address;
    iFile.block_pointers[2]= address;
    iFile.block_pointers[3]= address;


    char buffer[sizeof(Inode)+ 1];
    memset(buffer, 0, sizeof(Inode)+ 1);
    memcpy(buffer, &iFile, sizeof(Inode)+1);
    std::cout << "[MKLFS] writing iFile at offset: " << LOG_IFILE_OFFSET << std::endl;
    int res = Flash_Write((*l).flash, LOG_IFILE_OFFSET, 1, buffer);
    if (res) {
        std::cout << "[MKLFS] MKLFS WRITE FAIL" << std::endl;
    }
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
            std::cout << "[MKLFS] Error deleting file\n";
    }

    // Create the flash file
    int return_code = Flash_Create(file_name, wear_limit, no_of_segments * blocks_per_segment);

    //printf("%d",return_code);

    // If return code is not 0, something has gone wrong in creating the flash file
    if (return_code != 0) {
        std::cout << "[MKLFS] Failure! Exiting now...\n";
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv)
{
    std::cout << "[MKLFS] Starting MKLFS" << std::endl;
    // Initializing the default values for the parameters
	long no_of_segments = 50;
	long blocks_per_segment = 32;
    long sectors_per_block = 2;
    long wear_limit = 10000;
    char* file_name;

    // Get the flash file name
    file_name = argv[argc-1];

    int ch;
    int option_index = 0;

    // Figure out which flags have been explicitly stated
    while ((ch = getopt_long(argc, argv, "b:l:w:s:", long_options, &option_index)) != -1)
    {
        switch (ch) {            
            case 'b':
                sectors_per_block = atol(optarg);
                break;

            case 'l':
                blocks_per_segment = atol(optarg);
                break;
           
            case 's':
                no_of_segments = atol(optarg);
                break;
            
            case 'w':
                wear_limit = atol(optarg);
                break;
           
            case '?':
                break;

            default:
                std::cout << "[MKLFS] Invalid input arguments\n";
                abort ();
                break;         
        }
    }

    // Create the LFS file system
    lfs_create(no_of_segments, blocks_per_segment, wear_limit, file_name);
    unsigned int blocks;
    Log log = Log();
    log.flash = Flash_Open(file_name, 0, &blocks);
    std::cout << "[MKLFS] s: "<< no_of_segments << " l: " << blocks_per_segment << " b: " << sectors_per_block << " blks: " << blocks << " w: " << wear_limit;
    log.super_block = SuperBlock(no_of_segments, blocks_per_segment, sectors_per_block, blocks, wear_limit);
    lfs_init_superblock(&log);
    lfs_init_checkpoint(&log);
    lfs_init_segmentsummary(&log);
    lfs_init_inodemap(&log);
    Flash_Close(log.flash);

    std::cout<< "[MKLFS] MKLFS Complete!" << std::endl;
}

//usage script
// ./mklfs -l 32 -s 1024 -b 2 -w 1000 flash_test
