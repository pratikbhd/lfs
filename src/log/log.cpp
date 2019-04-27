#include <iostream>
#include "lfs_enums.h"
#include "log.h"
#include <string>

void Log::GetSuperBlock() {
    char buffer[FLASH_SECTOR_SIZE+2];
    super_block = SuperBlock();
    int res = Flash_Read(flash, LOG_SUPERBLOCK_OFFSET, 1, buffer);
    if (res) {
        std::cout <<"[Log] SuperBlock READ FAIL" << std::endl;
    }
    std::memcpy(&super_block, buffer, sizeof(SuperBlock)+1);
}

Checkpoint Log::GetCheckpoint(unsigned int sector){
        /* load checkpoints */
    char buffer[FLASH_SECTOR_SIZE+2];
    Checkpoint cp = Checkpoint();
    int res = Flash_Read(flash, sector, 1, buffer);
    if (res) std::cout << "[Log] Checkpoint Read failed at sector :" << sector << std::endl;
    std::memcpy(&cp, buffer, sizeof(Checkpoint));
    return cp;
}

void Log::InitializeCache(){
    log_end = new Segment(flash, super_block.bytesPerSegment, super_block.sectorsPerSegment, true);
    for(int i = 0; i< sizeof(cache) / sizeof(cache[0]); i++) {
        // cache the first 10 segments starting from segment 1
        cache[i] = new Segment(flash, super_block.bytesPerSegment, super_block.sectorsPerSegment, false);
        (*cache[i]).Load(i+1);
    }

    cache_round_robin= 0;
}

log_address Log::GetLogAddress(unsigned int segment_number, unsigned int block_number) {
    log_address address = {0, 0};
    address.segmentNumber = segment_number;
    address.blockOffset = block_number;
    return address;
}

block_usage Log::GetBlockUsage(log_address address) {
    //Block usage records are stored in the first block of the segment.
    //The first block of the segment serves as the segment summary block.
    //Thus we have to read the segment sequentially starting from the first block
    //to locate the requested block usage record.
    block_usage b = {0, 0, std::time(nullptr)};
    char data[summaryBlockBytes()];
    log_address read_address = GetLogAddress(address.segmentNumber, 0);
    Read(read_address, summaryBlockBytes(), data);
    char *o = data + (address.blockOffset * sizeof(block_usage));
    memcpy(&b, o, sizeof(block_usage));
    return b;
}

void Log::Read(log_address address, int length, char *buffer) {
    unsigned int offsetBytes = super_block.bytesPerBlock * address.blockOffset;
    if (offsetBytes + length > super_block.bytesPerSegment)
        throw "Log::Read() - Cannot read more than a segment! - " + std::to_string(offsetBytes + length);

    //check segment cache, if log end does not have the requested segment.
    if((*log_end).GetSegmentNumber() != address.segmentNumber) {
        // for (int i = 0; i < sizeof(cache) / sizeof(cache[0]); i++) {
        //     if ((*cache[i]).GetSegmentNumber() == address.segmentNumber){
        //         memcpy(buffer, (*cache[i]).data + offsetBytes, length);
        //         return;
        //     }
        // }
        
        // //segment not found in segment cache.
        // (*cache[cache_round_robin]).Load(address.segmentNumber);
        // memcpy(buffer, (*cache[cache_round_robin]).data + offsetBytes, length);
        // if (cache_round_robin >= (sizeof(cache) / sizeof(cache[0])) - 1){
        //     cache_round_robin = 0;
        // } else {
        //     cache_round_robin++;
        // }
        // return;

        (*log_end).Flush();
        RefreshCache((*log_end).GetSegmentNumber());
        (*log_end).Load(address.segmentNumber);
    }
    
    if (!(*log_end).IsLoaded())
        throw "Log::Read() - Attempting to read from a segment that is not loaded!!";

    memcpy(buffer, (*log_end).data + offsetBytes, length);   
}

void Log::Write(log_address address, int length, char *buffer) {
    unsigned int offsetBytes = super_block.bytesPerBlock * address.blockOffset;
    if (offsetBytes + length > super_block.bytesPerSegment)
        throw "Log::Write() - Cannot write more than a segment! - " + std::to_string(offsetBytes + length);

    //TODO handle Block wear.
    unsigned int w = 0;
    unsigned int b = address.segmentNumber * super_block.blocksPerSegment + address.blockOffset;
    Flash_GetWear(flash, b, &w);
    std::cout<< "[Log] Segment: " << address.segmentNumber << "Block: " << address.blockOffset << std::endl;
    std::cout<< "[Log] Block wear: " << w << std::endl;
    
    /*  Update Log End segment */
    if((*log_end).GetSegmentNumber() != address.segmentNumber) {
        (*log_end).Flush();
        RefreshCache((*log_end).GetSegmentNumber());
        (*log_end).Load(address.segmentNumber);
    }
    
    if (!(*log_end).IsLoaded())
        throw "Log::Write() - Attempting to write to a segment that is not loaded!!";

    memcpy((*log_end).data + offsetBytes, buffer, length);
}

void Log::Free(log_address address){
    unsigned int offsetBytes = super_block.bytesPerBlock * address.blockOffset;
    if (offsetBytes + super_block.bytesPerBlock > super_block.bytesPerSegment)
        throw "Log::Free() - block offset is out of bounds of segment! - " + std::to_string(address.blockOffset);

    ResetBlockUsage(address);

    /* blocks to be freed should be loaded to main memory */
    if((*log_end).GetSegmentNumber() != address.segmentNumber) {
        (*log_end).Flush();
        RefreshCache((*log_end).GetSegmentNumber());
        (*log_end).Load(address.segmentNumber);
    }

    if (!(*log_end).IsLoaded())
        throw "Log::Free() - Attempting to free a segment that is not loaded!!";

    memset((*log_end).data + offsetBytes, 0, super_block.bytesPerBlock);
}

//TODO:
//Log::Write -> should report wear and callers should handle this.
//Log::UpdateInode -> Should handle large files whose indirect block pointer list can exceed a block. Traverse block pointer list recursively. (low priority, can handle upto 1024 bytes of indirect blocks)
//Read the log end address from the checkpoint.

//Done:
//Internal:
//log_set_block_free -> ResetBlockUsage()
//log_set_block -> SetBlockUsage()
//log_write -> Write(log_address address, int length, char *buffer)
//log_get_higher_addr() -> getNextFreeBlock
//log_checkpoint_update() -> checkpoint()
//log_get_next_address() -> getNewLogEnd
//Required by Cleaner:
//log_get_block_record -> GetBlockUsage()
//
//Required by file layer:
//log_write_inode -> Write(Inode *in, unsigned int blockNumber, int length, const char* buffer)
//log_set_inode_addr -> UpdateInode()
//log_free -> Free()
//log_get_inode_addr -> GetLogAddress(Inode i, int index)
//log_create_addr -> GetLogAddress(unsigned int segment_number, unsigned int block_number)
//log_read -> Read()
//
//TODO:
//Log::Read -> Should support caching recently read segments. A round robin array should suffice.
//Log::Write -> should report wear and callers should handle this.
//Log::UpdateInode -> Should handle large files whose indirect block pointer list can exceed a block. Traverse block pointer list recursively. (low priority, can handle upto 1024 bytes of indirect blocks)
//Read the log end address from the checkpoint.