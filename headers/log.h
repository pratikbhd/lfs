#pragma once
#include "segment.h"
#include "flash.h"

class Log {
    public:
        Flash flash;
        SuperBlock super_block;
        Segment active_segment;
        log_address GetLogAddress(unsigned int segment_number, unsigned int block_number);
        block_usage GetBlockUsageRecord(log_address address);
        void SetActiveSegment(unsigned int segmentNumber);
        void Read (log_address address, int length, char *buffer);
        Log() = default;
};