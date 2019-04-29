#include <iostream>
#include <lfs_enums.h>
#include "log.h"

Log::~Log(){
    delete log_end;
    for(int i = 0; i< sizeof(cache) / sizeof(cache[0]); i++) {
        delete cache[i];
    } 
}

unsigned int Log::summaryBlockBytes(){
    return ((sizeof(block_usage) * super_block.blocksPerSegment) + 1);
}

unsigned int Log::SummaryBlockSize(){
    return ((sizeof(block_usage) * super_block.blocksPerSegment) / super_block.bytesPerBlock) + 1;
}

bool Log::SetBlockUsage(log_address address, block_usage record){
    if(address.segmentNumber == 0)
        return false;

    record.age = std::time(nullptr);
    char data[summaryBlockBytes()];
    log_address base = GetLogAddress(address.segmentNumber, 0);
    Read(base, summaryBlockBytes(), data);
    char *o = data + (address.blockOffset * sizeof(block_usage));
    memcpy(o, &record, sizeof(block_usage));
    Write(base, summaryBlockBytes(), data);
    return true;
}

bool Log::ResetBlockUsage(log_address address){
    block_usage b;
    b.inum = static_cast<unsigned int>(reserved_inum::NOINUM);
    b.use = static_cast<char>(usage::FREE);
    return SetBlockUsage(address, b);
}


log_address Log::GetNextFreeBlock(log_address current){
    if (current.segmentNumber == 0) {
        current.segmentNumber = 1; 
    }
    if(current.blockOffset+1 >= super_block.blocksPerSegment) {
        current.blockOffset = SummaryBlockSize();
        current.segmentNumber = getNextFreeSegment(current.segmentNumber);
    } else {
        current.blockOffset = current.blockOffset + 1;
        current.segmentNumber = current.segmentNumber;
    }
    return current;
}

unsigned int Log::getNextFreeSegment(unsigned int segmentNumber){
        log_address finder = {segmentNumber +1, SummaryBlockSize()};

        if (finder.segmentNumber >= super_block.segmentCount) {
            finder.segmentNumber = 1;
        }
        
        while(finder.segmentNumber != segmentNumber) {
            bool freeSegment = true;
            while (finder.blockOffset < super_block.blocksPerSegment && freeSegment) {
                block_usage b = GetBlockUsage(finder);
                if (b.use == static_cast<char>(usage::INUSE)){
                    freeSegment = false;
                }
                finder.blockOffset++;
            }

            if (freeSegment) return finder.segmentNumber;
            else {
                if (finder.segmentNumber >= super_block.segmentCount -1) finder.segmentNumber = 1;
                else finder.segmentNumber++;
            }
        }

        if(finder.segmentNumber == segmentNumber) throw "[LOG] FLASH is FULL, no free segments available";
}

int Log::GetUsedBlockCount(){
    int i = 0;
    int sum = 0;
    for(; i < super_block.segmentCount; i++) {
        sum += super_block.blocksPerSegment - GetFreeBlockCount(i);
    }
    return sum;
}

int Log::GetFreeBlockCount(int segmentNumber){
    int free = 0;
    char buffer[summaryBlockBytes()];
    Read(GetLogAddress(segmentNumber, 0), summaryBlockBytes(), buffer);
    int j = 0;
    for(; j < super_block.segmentCount; j++) {
        block_usage *br = (block_usage*) (buffer + j * sizeof(block_usage));
        if(br->use == static_cast<char>(usage::FREE))
            ++free;
    }
    return free;
}

void Log::RefreshCache(int segmentNumber){
    std::cout << "[LOG] Check Cache for Refresh: " << segmentNumber <<std::endl;
    for(int i = 0; i< sizeof(cache) / sizeof(cache[0]); i++) {
        if((*cache[i]).GetSegmentNumber() == segmentNumber){
            std::cout << "[LOG] Refreshing Cache: " << segmentNumber <<std::endl;
            (*cache[i]).Load(segmentNumber);
        }
    }
}
