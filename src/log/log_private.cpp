#include <iostream>
#include <lfs_enums.h>
#include "log.h"

Log::~Log(){
    delete log_end;
}

unsigned int Log::summaryBlockSize(){
    return ((sizeof(block_usage) * super_block.blocksPerSegment) / super_block.bytesPerBlock) + 1;
}

unsigned int Log::summaryBlockBytes(){
    return ((sizeof(block_usage) * super_block.blocksPerSegment) + 1);
}

log_address Log::getNextFreeBlock(log_address current){

    if (current.segmentNumber == 0) {
        current.segmentNumber = 1; 
    }

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
