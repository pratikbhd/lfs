#pragma once
#include "segment.h"
#include "flash.h"

//TODO: Implement copy constructor / rule of three constructors.
class Log {
    public:
        Flash flash;
        SuperBlock super_block;
        Segment *log_end;

        void InitializeCache();

        log_address GetLogAddress(unsigned int segment_number, unsigned int block_number);
        block_usage GetBlockUsageRecord(log_address address);
        
        void Read (log_address address, int length, char *buffer);
        Log() = default;
        ~Log();
};