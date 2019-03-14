#include <iostream>
#include "log.h"

Log::~Log(){
    delete log_end;
}

void Log::InitializeCache(){
    log_end = new Segment(flash, super_block.bytesPerSegment, super_block.sectorsPerSegment, true);
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
    /*  Update Active segment */
    if((*log_end).GetSegmentNumber() != address.segmentNumber) {
        (*log_end).Flush();
        (*log_end).Load(address.segmentNumber);
    } 
    
    //TODO Add support to read from the segment cache

    memcpy(buffer, (*log_end).data, length);
}

//TODO:
//log_free
//log_set_inode_addr
//log_get_inode_addr
//log_write_inode -> write entire file with inode
//
//Done:
//log_create_addr -> GetLogAddress
//log_read -> Read
