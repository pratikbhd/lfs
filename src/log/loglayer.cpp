/*
 ============================================================================
 Name        : loglayer.cpp
 Author      : Rahul Roy Mattam
 Version     : 1.0
 Description : The log layer for LFS Project
 ============================================================================

 **  Usage  *****************************************************************
  loglayer file
  where file is the name of the virtual flash file to create
*/
#include "flash.h"
#include <string>
#include <iostream>

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
    unsigned int blocks;
    auto flash = Flash_Open(file_name, 0,&blocks);
    std::cout << blocks;
    Flash_Close(flash);
}