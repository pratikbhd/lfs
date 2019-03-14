#include <iostream>
#include "log.h"

void Log::GetSuperBlock() {
    char buffer[FLASH_SECTOR_SIZE+2];
    super_block = SuperBlock();
    int res = Flash_Read(flash, LOG_SUPERBLOCK_OFFSET, 1, buffer);
    if (res) {
        std::cout <<"SuperBlock READ FAIL" << std::endl;
    }
    std::memcpy(&super_block, buffer, sizeof(SuperBlock)+1);
}

Checkpoint Log::GetCheckpoint(unsigned int sector){
        /* load checkpoints */
    char buffer[FLASH_SECTOR_SIZE+2];
    Checkpoint cp = Checkpoint();
    int res = Flash_Read(flash, sector, 1, buffer);
    if (res) std::cout << "Checkpoint Read failed at sector :" << sector << std::endl;
    std::memcpy(&cp, buffer, sizeof(Checkpoint)+1);
    return cp;
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

block_usage Log::GetBlockUsage(log_address address) {
    //Block usage records are stored in the first block of the segment.
    //The first block of the segment serves as the segment summary block.
    //Thus we have to read the segment sequentially starting from the first block
    //to locate the requested block usage record.
    block_usage b = {0, 0};
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
        throw "Log::Read() - Cannot read more than a segment! - " + (offsetBytes + length);

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
    if (offsetBytes + length > super_block.bytesPerSegment)
        throw "Log::Write() - Cannot write more than a segment! - " + (offsetBytes + length);

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

void Log::Free(log_address address){
    unsigned int offsetBytes = super_block.bytesPerBlock * address.blockOffset;
    if (offsetBytes + super_block.bytesPerBlock > super_block.bytesPerSegment)
        throw "Log::Free() - block offset is out of bounds of segment! - " + address.blockOffset;

    resetBlockUsage(address);

    /* blocks to be freed should be loaded to main memory */
    if(!(*log_end).GetSegmentNumber() != address.segmentNumber) {
        (*log_end).Flush();
        (*log_end).Load(address.segmentNumber);
    }
    
    memset((*log_end).data + offsetBytes, 0, super_block.bytesPerBlock);
}

bool Log::UpdateInode(Inode *i, int index, log_address address) {
    block_usage b;
    b.inum = (*i).inum;
    b.use = static_cast<char>(usage::INUSE);

    if(index < 0) {
        return false;
    } else if(index < 4) {
        if((*i).block_pointers[index].segmentNumber > 0) {
            resetBlockUsage((*i).block_pointers[index]);
        }
        (*i).block_pointers[index] = address;
    } else {
        char data[super_block.bytesPerBlock];
        unsigned int offsetBytes = (index - 4) * sizeof(log_address);
        if (offsetBytes + sizeof(log_address) > super_block.bytesPerBlock)
            throw "Log::UpdateInode - the indirect block pointer data exceeded a block size. Not supported as of now.";

        if((*i).indirect_block.segmentNumber > 0) {
            Read((*i).indirect_block, super_block.bytesPerBlock, data);

            resetBlockUsage((*i).indirect_block);
            (*i).indirect_block = getNewLogEnd();

            log_address old = {0,0};
            memcpy(&old, data+offsetBytes, sizeof(log_address));
            resetBlockUsage(old);
        } else {
            (*i).indirect_block = getNewLogEnd();
        }

        memcpy((data + offsetBytes), &address, sizeof(log_address));
        //write back indirect block at the log end.
        Write((*i).indirect_block, super_block.bytesPerBlock, data);
        //update block usage of indirect block at log end.
        setBlockUsage((*i).indirect_block, b);
    }

    //update block usage of new block pointer of inode in flash
    setBlockUsage(address, b);
    return true;
}

//TODO:
//Required by file layer:
//log_get_inode_addr
//log_write_inode -> write entire file with inode
//
//Done:
//Internal:
//log_set_block_free -> resetBlockUsage()
//log_set_block -> setBlockUsage()
//log_write -> Write()
//log_get_higher_addr() -> getNextFreeBlock
//log_checkpoint_update() -> checkpoint()
//log_get_next_address() -> getNewLogEnd
//
//Required by file layer:
//log_set_inode_addr -> UpdateInode()
//log_free -> Free()
//log_create_addr -> GetLogAddress()
//log_read -> Read()
//
//TODO:
//Log::Read -> Should support caching recently read segments. A round robin array should suffice.
//Log::Write -> should report wear and callers should handle this.
//Log::UpdateInode -> Should handle large files whose indirect block pointer list can exceed a block. Traverse block pointer list recursively. (low priority, can handle upto 1024 bytes of indirect blocks)