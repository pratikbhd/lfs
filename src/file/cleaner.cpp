#include <iostream>
#include <algorithm>
#include "file.hpp"
#include "cost_benefit.hpp"
#include "color.hpp"

Inode File::getInode(int inum){
    int blocknum = (inum)/((log.super_block.bytesPerBlock)/(sizeof(Inode)));
    log_address addr = GetLogAddress(iFile, blocknum);

    char buf[log.super_block.bytesPerBlock];
    log.Read(addr, log.super_block.bytesPerBlock, buf);
    
    int offset = ((inum)%((log.super_block.bytesPerBlock)/(sizeof(Inode))))*sizeof(Inode);

    Inode result = Inode();
    memcpy(&result, (buf+offset), sizeof(Inode));
    return result; 
}

void File::updateInode(Inode in, log_address before, log_address after) {
    int wi = 0;
    while (GetLogAddress(in, wi).segmentNumber != 0 ) {
        log_address address = GetLogAddress(in, wi);
        if(address.segmentNumber == before.segmentNumber &&
           address.blockOffset   == before.blockOffset ) {
            updateInode(&in, wi, after);
            break;
        }
        wi++;
    }
}

bool File::cleanSegments(std::vector<unsigned int> segments) {
    Color::Modifier yel(Color::FG_YELLOW);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << yel << "[CLEANER] cleanSegments() BEGIN" << def <<std::endl;
    /* Iterate over all of the segments passed to the function */
    for(unsigned int i = 0; i< segments.size(); i++) {
        std::cout << yel << "[CLEANER] cleaning segment: " << segments[i] << def <<std::endl;
        for (unsigned int j = log.SummaryBlockSize(); j < log.super_block.blocksPerSegment; j++) {
            log_address block_address = log.GetLogAddress(segments[i], j);
            block_usage move = log.GetBlockUsage(block_address);
            if (move.use == static_cast<char>(usage::INUSE)){
                //move the block to log end.
                char move_buffer[log.super_block.bytesPerBlock];
                log.Read(block_address, log.super_block.bytesPerBlock, move_buffer);
                log_address log_end = getNewLogEnd();
                log.Write(log_end, log.super_block.bytesPerBlock, move_buffer);
                log.SetBlockUsage(log_end, move);
                log.ResetBlockUsage(block_address);
                /* Update inode pointer */
                Inode in = getInode(move.inum);
                updateInode(in, block_address, log_end);
	            fileWrite(&iFile, in.inum * sizeof(Inode), sizeof(Inode), &in);
            }
        }
        log.super_block.usedSegments--;  
    }
    std::cout << yel << "[CLEANER] cleanSegments() END" << def <<std::endl;
    return true;
}

bool File::selectSegments(int segmentsToFree){
        CostBenefit ratios[log.super_block.segmentCount-1];
        //ignore segment zero.
        for (int i = 1; i < log.super_block.segmentCount; i++) {
            double utilization = ((log.super_block.blockCount - log.SummaryBlockSize()) - log.GetFreeBlockCount(i))/ (double)(log.super_block.blockCount - log.SummaryBlockSize());
            struct tm y2k = {0};
            double seconds;
            y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
            y2k.tm_year = 115; y2k.tm_mon = 0; y2k.tm_mday = 1;
            std::time_t newest = std::mktime(&y2k);
            for (int j = log.SummaryBlockSize(); j < log.super_block.blocksPerSegment; j++) {
                        log_address block = log.GetLogAddress(i, j);
                        block_usage br = log.GetBlockUsage(block);
                        if (std::difftime(br.age, newest) > 0) newest = br.age;
            }
            ratios[i-1] = CostBenefit();
            ratios[i-1].segmentNumber = i;
            ratios[i-1].utilization = utilization;
            ratios[i-1].newest = newest;
            ratios[i-1].timeDiff = difftime(newest,mktime(&y2k));

            ratios[i-1].score = (((1 - utilization) * difftime(newest,mktime(&y2k))) / (1 + utilization));
        }

        std::sort(ratios, ratios + (log.super_block.segmentCount-1),
          [](CostBenefit const & a, CostBenefit const & b) -> bool
          { return a.score > b.score; } );

        std::vector<unsigned int> selected;
        for (int i = 0; i<segmentsToFree; i++){
            selected.push_back(ratios[i].segmentNumber);
        }
        return cleanSegments(selected);
}

/**
 * Perform the clean operation.
 */
bool File::clean() {	
    std::cout << "[CLEANER] CLEANER_CLEAN BEGIN" <<std::endl;
    std::cout <<"[CLEANER] CLEANER USED SEGS: " << log.super_block.usedSegments << std::endl;
    std::cout <<"[CLEANER] CLEANER TOTAL SEGS: " << log.super_block.segmentCount << std::endl;
    std::cout <<"[CLEANER] CLEANER START: " << state.startCleaner << std::endl;
    std::cout <<"[CLEANER] CLEANER STOP: "<< state.stopCleaner << std::endl;
    bool rv = true;
    if(state.startCleaner >= (log.super_block.segmentCount - log.super_block.usedSegments)) {
        int segmentsToFree = state.stopCleaner - state.startCleaner;
        Color::Modifier yel(Color::FG_YELLOW);
        Color::Modifier def(Color::FG_DEFAULT);
        std::cout << yel << "[CLEANER] CLEANER will clean: " << segmentsToFree << def << std::endl;
        rv = selectSegments(segmentsToFree);
        checkpoint();
    } else {
        std :: cout << "[CLEANER] clean(): cleaner does not have to run yet." << std::endl;
    }
    return rv;
}
