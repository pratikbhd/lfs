#include <iostream>
#include "file.hpp"

Inode File::getInode(int inum){
    int blocknum = (inum)/((log.super_block.bytesPerBlock)/(sizeof(Inode)));
    log_address addr = getLogAddress(iFile, blocknum);

    char buf[log.super_block.bytesPerBlock];
    log.Read(addr, log.super_block.bytesPerBlock, buf);
    
    int offset = ((inum)%((log.super_block.bytesPerBlock)/(sizeof(Inode))))*sizeof(Inode);

    Inode result = Inode();
    memcpy(&result, (buf+offset), sizeof(Inode));
    return result; 
}

void File::updateInode(Inode in, log_address before, log_address after) {
    int wi = 0;
    while (getLogAddress(in, wi).segmentNumber != 0 ) {
        log_address address = getLogAddress(in, wi);
        if(address.segmentNumber == before.segmentNumber &&
           address.blockOffset   == before.blockOffset ) {
            updateInode(&in, wi, after);
            break;
        }
        wi++;
    }
}


bool File::mergeSegments(std::vector<unsigned int> segments) {
    std::cout << "CLEANER: Merging free Segments" << std::endl;
    
    unsigned int j  = log.SummaryBlockSize();
    unsigned int iw = 0, jw = log.SummaryBlockSize();;
    block_usage br     = block_usage();
    block_usage move   = block_usage();
    char move_buf[log.super_block.bytesPerBlock];
    bool found_free_slot = false;
    log_address addrr;
    log_address addrw;
   
    /* Iterate over all of the segments passed to the function */
    for(unsigned int i = 0; i< segments.size(); i++) {
        while (j < log.super_block.blocksPerSegment) {
            addrw = log.GetLogAddress(segments[i], j);
            
            std::cout << "CLEANER: Merge Segments LOOP (i,j) = " <<segments[i] << ","<< j << std::endl;

            /* Find next free slot */
            if (!found_free_slot) {
                while (iw < segments.size()) {
                    while (jw < log.super_block.blocksPerSegment) {
                        addrw = log.GetLogAddress(segments[iw], jw);
                        br = log.GetBlockUsage(addrw);
                        if (br.use == static_cast<char>(usage::FREE)) {
                            found_free_slot = true;
                            break;
                        }
                        jw++;
                    }
                    if(found_free_slot) 
                        break;
                    jw = log.SummaryBlockSize();
                    iw++;
                }
            }
           
            /* check if we found a block to move */
            move = log.GetBlockUsage(addrr);

            /* move a block if so */
            if (move.use == static_cast<char>(usage::INUSE) && found_free_slot == true) {

                /* Update usage block */
                log.Read(addrr, log.super_block.bytesPerBlock, move_buf);
                log.Write(addrw, log.super_block.bytesPerBlock, move_buf);
                log.SetBlockUsage(addrw, br);
                log.ResetBlockUsage(addrr);

                /* Update inode pointer */
                Inode in = getInode(move.inum);
                updateInode(in, addrr, addrw);
	            fileWrite(&iFile, in.inum * sizeof(Inode), sizeof(Inode), &in);

                found_free_slot = false;
            }
            j++;
        }
        j = log.SummaryBlockSize();
        i++;
    }
    
    log.super_block.usedSegments--;  
    std::cout << "CLEANER_MERGE_SEGMENTS END" << std::endl;

    return true;
}


bool File::cleanSegment() {
    std::cout << "CleanBlocks BEGIN" << std::endl;
    int total = 0;
    std::vector<unsigned int> freeBlocks;
    for (int i = 0; i <= log.super_block.segmentCount; i++) {
        int local_total = log.GetFreeBlockCount(i);
            
        /* Note too full, can merge */
        if ( local_total > 0 ) {
            total += local_total;
            freeBlocks.push_back(i);
        }

        std::cout << "FREE COUNT TOTAL: " << total << std::endl;

        /* Check if we have found enough free blocks to do merging */
        if (total > log.super_block.blocksPerSegment && i > 1) {
            break;
        }
    }
    mergeSegments(freeBlocks);
    std::cout << "CleanBlocks END" << std::endl;
    return true;
}

/**
 * Perform the clean operation.
 */
bool File::clean() {
	
    std::cout << "CLEANER_CLEAN BEGIN" <<std::endl;
    std::cout <<"CLEANER USED SEGS: " << log.super_block.usedSegments << std::endl;
    std::cout <<"CLEANER TOTAL SEGS: " << log.super_block.segmentCount << std::endl;
    std::cout <<"CLEANER START: " << state.startCleaner << std::endl;
    std::cout <<"CLEANER STOP: "<< state.stopCleaner << std::endl;

    bool rv = true;

    if(state.startCleaner >= (log.super_block.segmentCount - log.super_block.usedSegments)) {
        int segmentsToFree = state.stopCleaner - state.startCleaner;
        std::cout << "CLEANER will clean: " << segmentsToFree << std::endl;
        while(segmentsToFree > 0) {
            bool didClean = cleanSegment();
            if(!didClean) {
                std::cout << "clean(): did not find enough free blocks for making a new clean segment! Segments left to clean:" << segmentsToFree << std::endl;
                rv = false;
                break;
            }
            segmentsToFree--;
        }
    } else {
        std :: cout << "clean(): cleaner does not have to run yet." << std::endl;
    }

    //TODO checkpoint?
    return rv;
}
