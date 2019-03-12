#pragma once
#include "segment.h"

class Log {
    public:
        SuperBlock super_block;
        log_address GetLogAddress(unsigned int segment_number, unsigned int block_number);

};