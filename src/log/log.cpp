#include <iostream>
#include "log.h"

Log::~Log(){
    delete log_end;
}

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

unsigned int Log::summaryBlockSize(){
    return ((sizeof(block_usage) * super_block.blocksPerSegment) / super_block.bytesPerBlock) + 1;
}

unsigned int Log::summaryBlockBytes(){
    return ((sizeof(block_usage) * super_block.blocksPerSegment) + 1);
}

log_address Log::getNextFreeBlock(log_address current){
    if(current.blockOffset+1 >= super_block.blocksPerSegment) {
        current.blockOffset = summaryBlockSize();
        current.segmentNumber = current.segmentNumber + 1;
        //TODO: this line should happen right before the checkpoint event.
        //super_block.usedSegments++;
    } else {
        current.blockOffset = current.blockOffset + 1;
        current.segmentNumber = current.segmentNumber;
    }

    if(current.segmentNumber >= super_block.segmentCount) {
        current.segmentNumber = 1;
        current.blockOffset   = summaryBlockSize();
    }
    return current;
}

log_address Log::getNewLogEnd(){
    block_usage b = {0,static_cast<char>(usage::INUSE)};
    log_address finder = log_end_address;

    while (b.use == static_cast<char>(usage::INUSE)) {
        finder = getNextFreeBlock(finder);
        //Getting a block in a new segment will force flushing the current segment to flash.
        //Thus it is not necessary to flush a segment again in checkpoint, 
        //But we still do it as of now to maintain semantics of the checkpoint operation.
        b = GetBlockUsage(finder); 
    }

    if (log_end_address.segmentNumber != finder.segmentNumber){
        operation_count = max_operations; //force the checkpoint block to flash
    }

    log_end_address = finder;
    checkpoint();    
    return log_end_address;
}

void Log::checkpoint() {
    operation_count++;
    if(operation_count < max_operations) {
        return;
    }

    Checkpoint current, other;
    bool first;
    if (cp1.time < cp2.time) {
        current = cp1;
        other = cp2;
        first = true;
    } else {
        current = cp2;
        other = cp1;
        first = false;
    }
    current.address = log_end_address;
    current.time = other.time + 1;

    /* Flush log end to flash */
    (*log_end).Flush();

    /* Erase first segment for flash metadata */
    int res = Flash_Erase(flash, 0, 1);
    if (res) {
        std::cout << "Erasing superblock on flash failed"; 
        return;
    }
    
    /* TODO save ifile */

    res = Flash_Write(flash, LOG_SUPERBLOCK_OFFSET, 1, &super_block);
    if (res) {
        std::cout << "superblock write failed";
        return;
    }

    res = Flash_Write(flash, first ? LOG_CP1_OFFSET : LOG_CP2_OFFSET, 1, &current);
    if (res) {
        std::cout << "checkpoint write failed";
        return;
    }

    operation_count = 0;
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

bool Log::setBlockUsage(log_address address, block_usage record){
    if(address.segmentNumber == 0)
        return false;

    char data[summaryBlockBytes()];
    log_address base = GetLogAddress(address.segmentNumber, 0);
    Read(base, summaryBlockBytes(), data);
    char *o = data + (address.blockOffset * sizeof(block_usage));
    memcpy(o, &record, sizeof(block_usage));
    Write(base, summaryBlockBytes(), data);
    return true;
}

bool Log::resetBlockUsage(log_address address){
    block_usage b;
    b.inum = static_cast<unsigned int>(reserved_inum::NOINUM);
    b.use = static_cast<char>(usage::FREE);
    return setBlockUsage(address, b);
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
//log_cache_free
//Required by file layer:
//log_free
//log_set_inode_addr
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
//log_create_addr -> GetLogAddress
//log_read -> Read
//
//TODO:
//Log::Read -> Should support caching recently read segments. A round robin array should suffice.
//Log::Write -> should report wear and callers should handle this.
