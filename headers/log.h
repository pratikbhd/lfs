#pragma once
#include "segment.h"
#include "flash.h"

//TODO: Implement copy constructor / rule of three constructors for pointer members. 
//Avoid invoking Copy/Reallocate this class in the mean time.
//Should be fine since this class is meant to serve as a Singleton. Only pass by reference.
class Log {
    private:
        log_address log_end_address;
        unsigned int operation_count;
        unsigned int max_operations = 1000;

        unsigned int summaryBlockSize();
        unsigned int summaryBlockBytes();

        log_address getNextFreeBlock(log_address current);
        log_address getNewLogEnd();
        //Trigger a checkpoint to flash.
        void checkpoint();

        //Set a block usage record for a log address. Returns false if the operation is invalid or fails.
        bool setBlockUsage(log_address address, block_usage record);
        //Reset a block usage record to be unused and have no inum associated with it.
        bool resetBlockUsage(log_address address);
    public:
        Flash flash;
        SuperBlock super_block;
        Segment *log_end;

        Checkpoint cp1;
        Checkpoint cp2;

        //Get the SuperBlock from the flash.
        void GetSuperBlock();

        //Get the checkpoint block from the flash saved at sector.
        Checkpoint GetCheckpoint(unsigned int sector);

        //Initialize meta for the class members that hold the tail end of the log and segment caches in memory.
        void InitializeCache();

        //Get a log address object for a segment number and block number.
        log_address GetLogAddress(unsigned int segment_number, unsigned int block_number);
        //Get a Block usage record for a log address. Caches the log segment referred for Read if it is not the tail end segment.
        block_usage GetBlockUsage(log_address address);

        //Read from the tail end segment of the log or any other segments cached starting at address. length is in Bytes, cannot exceed segment size.
        void Read (log_address address, int length, char *buffer);
        //Write to the tail end segment of the log starting at address. length is in Bytes, cannot exceed segment size.
        void Write(log_address address, int length, char *buffer);
        //Free a file block located at address in the flash.
        void Free(log_address address);

        //default constructor used so that mklfs, loglayer and lfsck are flexible.
        Log() = default;
        ~Log();
};