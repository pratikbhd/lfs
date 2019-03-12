#include "log.h"

log_address Log::GetLogAddress(unsigned int segment_number, unsigned int block_number){
    log_address address = {0, 0};
    address.segmentNumber = segment_number;
    address.blockOffset = block_number;
    return address;
}