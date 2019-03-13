#include <iostream>
#include "log.h"

log_address Log::GetLogAddress(unsigned int segment_number, unsigned int block_number){
    log_address address = {0, 0};
    address.segmentNumber = segment_number;
    address.blockOffset = block_number;
    return address;
}

block_usage Log::GetBlockUsageRecord(log_address address){
    block_usage b = {0, 0};
    char data[(sizeof(b) * super_block.blocksPerSegment)+1];
    log_address read_address = GetLogAddress(address.segmentNumber, 0);
    Read(read_address, (sizeof(b) * super_block.blocksPerSegment)+1, data);
    char *o = data + (address.blockOffset * sizeof(b));
    memcpy(&b, o, sizeof(b));
    return b;
}

void Log::SetActiveSegment(unsigned int segmentNumber){
    unsigned int sector_num = (segmentNumber)*(super_block.sectorsPerSegment);
    active_segment = Segment(super_block.bytesPerSegment, segmentNumber);
    int res = Flash_Read(flash, 
                sector_num, 
                super_block.sectorsPerSegment, 
                active_segment.data.get()
              );

    if(res)
        std::cout << "READ FAIL!" << std::endl;
}

void Log::Read (log_address address, int length, char *buffer)
{
    //int read_index;

    /*  Update Active segment */
    // if(active_segment.GetSegmentNumber() != address.segmentNumber) {
    //     //log_cache_flush_index(segment_cache_current_index);
    //     SetActiveSegment(address.segmentNumber);
    // } 
    
    //read_index = log_cache_get_index(segmentNum);
   
    // char *read_offset = (segment_cache +
    //                     (((lInfo->bytesPerSegment)*read_index) + 
    //                     ((lInfo->bytesPerBlock)*block)));
    

    unsigned int sector_num = (address.segmentNumber)*(super_block.sectorsPerSegment);

    char segment[super_block.bytesPerSegment+1];
    int res = Flash_Read(flash, 
                sector_num, 
                super_block.sectorsPerSegment, 
                segment
              );

    if(res)
        std::cout << "READ FAIL!" << std::endl;

    memcpy(buffer, segment, length);
}