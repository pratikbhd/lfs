#pragma once
#include "segment.h"
#include "flash.h"

//TODO: Implement copy constructor / rule of three constructors for pointer members. 
//Avoid invoking Copy/Reallocate this class in the mean time.
//Should be fine since this class is meant to serve as a Singleton. Only pass by reference.
class Log {
    public:
        Flash flash;
        SuperBlock super_block;
        Segment *log_end;

        //Initialize meta for the class members that hold the tail end of the log and segment caches in memory.
        void InitializeCache();

        //Get a log address object for a segment number and block number
        log_address GetLogAddress(unsigned int segment_number, unsigned int block_number);
        //Get a Block usage record for a log address. Caches the log segment referred for Read if it is not the tail end segment.
        block_usage GetBlockUsageRecord(log_address address);
        //Read from the tail end segment of the log or any other segments cached starting at address. length is in Bytes, cannot exceed segment size.
        void Read (log_address address, int length, char *buffer);
        //Write to the tail end segment of the log starting at address. length is in Bytes, cannot exceed segment size.
        void Write(log_address address, int length, char *buffer);
        Log() = default;
        ~Log();
};