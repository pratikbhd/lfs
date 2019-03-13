#include <iostream>
#include "log.h"

Log::~Log(){
    delete active_segment;
}

void Log::InitializeCache(){
    active_segment = new Segment(flash, super_block.bytesPerSegment, super_block.sectorsPerSegment);
}

log_address Log::GetLogAddress(unsigned int segment_number, unsigned int block_number) {
    log_address address = {0, 0};
    address.segmentNumber = segment_number;
    address.blockOffset = block_number;
    return address;
}

block_usage Log::GetBlockUsageRecord(log_address address) {
    block_usage b = {0, 0};
    char data[(sizeof(b) * super_block.blocksPerSegment)+1];
    log_address read_address = GetLogAddress(address.segmentNumber, 0);
    Read(read_address, (sizeof(b) * super_block.blocksPerSegment)+1, data);
    char *o = data + (address.blockOffset * sizeof(b));
    memcpy(&b, o, sizeof(b));
    return b;
}

void Log::Read (log_address address, int length, char *buffer) {
    //int read_index;

    /*  Update Active segment */
    if((*active_segment).GetSegmentNumber() != address.segmentNumber) {
        //log_cache_flush_index(segment_cache_current_index);
        (*active_segment).Load(address.segmentNumber);
    } 
    
    //read_index = log_cache_get_index(segmentNum);
   
    // char *read_offset = (segment_cache +
    //                     (((lInfo->bytesPerSegment)*read_index) + 
    //                     ((lInfo->bytesPerBlock)*block)));

    memcpy(buffer, (*active_segment).data, length);
}