#pragma once
#include "segment.h"
#include "flash.h"

//TODO: Implement copy constructor / rule of three constructors for pointer members. 
//Avoid invoking Copy/Reallocate this class in the mean time.
//Should be fine since this class is meant to serve as a Singleton. Only pass by reference.

struct inputState{
    char* lfsFile;
    // int cacheSize;
    int interval;
    int startCleaner;
    int stopCleaner;
};

class Log {
    private:
        unsigned int summaryBlockBytes();
    public:
        //log address of the current log end.
        log_address Log_end_address;

        void RefreshCache(int segmentNumber);

        log_address GetNextFreeBlock(log_address current);

        unsigned int SummaryBlockSize();
        //Set a block usage record for a log address. Returns false if the operation is invalid or fails.
        bool SetBlockUsage(log_address address, block_usage record);
        //Reset a block usage record to be unused and have no inum associated with it.
        bool ResetBlockUsage(log_address address);

        Flash flash;
        SuperBlock super_block;
        Segment *log_end;

        Segment* cache[10];
        int cache_round_robin;

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

        //Get the count of free blocks from the specified segment.
        int GetFreeBlockCount(int segmentNumber);

        //Get the count of used blocks available in the entire flash.
        int GetUsedBlockCount();

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