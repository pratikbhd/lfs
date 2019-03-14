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
    //Block usage records are stored in the first block of the segment.
    //The first block of the segment serves as the segment summary block.
    //Thus we have to read the segment sequentially starting from the first block
    //to locate the requested block usage record.
    block_usage b = {0, 0};
    char data[(sizeof(b) * super_block.blocksPerSegment)+1];
    log_address read_address = GetLogAddress(address.segmentNumber, 0);
    Read(read_address, (sizeof(b) * super_block.blocksPerSegment)+1, data);
    char *o = data + (address.blockOffset * sizeof(b));
    memcpy(&b, o, sizeof(b));
    return b;
}

void Log::Read (log_address address, int length, char *buffer) {
    unsigned int offsetBytes = super_block.bytesPerBlock * address.blockOffset;
    if (offsetBytes > super_block.bytesPerSegment)
        throw "Log::Read() - Cannot read more than a segment! - " + offsetBytes;

    /*  Update Log End segment */
    if((*log_end).GetSegmentNumber() != address.segmentNumber) {
        (*log_end).Flush();
        (*log_end).Load(address.segmentNumber);
    } 
    
    //TODO Add support to read from the segment cache

    memcpy(buffer, (*log_end).data + offsetBytes, length);
}

void Log::Write(log_address address, int length, char *buffer) {
    unsigned int offsetBytes = super_block.bytesPerBlock * address.blockOffset;
    if (offsetBytes > super_block.bytesPerSegment)
        throw "Log::Write() - Cannot write more than a segment! - " + offsetBytes;

    //TODO handle Block wear.
    unsigned int w = 0;
    unsigned int b = address.segmentNumber * super_block.blocksPerSegment + address.blockOffset;
    Flash_GetWear(flash, b, &w);
    std::cout<< "Segment: " << address.segmentNumber << "Block: " << address.blockOffset << std::endl;
    std::cout<< "Block wear: " << w << std::endl;
    
    /*  Update Log End segment */
    if((*log_end).GetSegmentNumber() != address.segmentNumber) {
        (*log_end).Flush();
        (*log_end).Load(address.segmentNumber);
    }
    
    memcpy((*log_end).data + offsetBytes, buffer, length);
}

//TODO:
//Internal:
//log_get_next_address()
//Required by file layer:
//log_free
//log_set_inode_addr
//log_get_inode_addr
//log_write_inode -> write entire file with inode
//
//Done:
//Internal:
//log_write
//Required by file layer:
//log_create_addr -> GetLogAddress
//log_read -> Read
