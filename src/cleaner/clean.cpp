#include <iostream>
#include "cleaner.hpp"

Cleaner::Cleaner(inputState state): file(File(state.lfsFile)), state(state) {
	
}

Inode Cleaner::GetInode(int inum){
    int blocknum = (inum)/((file.log.super_block.bytesPerBlock)/(sizeof(Inode)));
    log_address addr = file.log.GetLogAddress(file.log.iFile, blocknum);

    char buf[file.log.super_block.bytesPerBlock];
    file.log.Read(addr, file.log.super_block.bytesPerBlock, buf);
    
    int offset = ((inum)%((file.log.super_block.bytesPerBlock)/(sizeof(Inode))))*sizeof(Inode);

    Inode result = Inode();
    memcpy(&result, (buf+offset), sizeof(Inode));
    return result; 
}

void Cleaner::UpdateInode(Inode in, log_address before, log_address after) {
    int wi = 0;
    while (file.log.GetLogAddress(in, wi).segmentNumber != 0 ) {
        log_address address = file.log.GetLogAddress(in, wi);
        if(address.segmentNumber == before.segmentNumber &&
           address.blockOffset   == before.blockOffset ) {
            file.log.UpdateInode(&in, wi, after);
            break;
        }
        wi++;
    }
}


bool Cleaner::MergeSegments(int *segments)
{
    std::cout << "CLEANER: Merging free Segments" << std::endl;
    
    int i  = 1, j  = file.log.SummaryBlockSize();
    int iw = 0, jw = file.log.SummaryBlockSize();;
    block_usage br     = block_usage();
    block_usage move   = block_usage();
    char move_buf[file.log.super_block.bytesPerBlock];
    bool found_free_slot = false;
    log_address addrr;
    log_address addrw;
   
    /* Iterate over all of the segments passed to the function */
    while (*(segments + i) != 0) {
        while (j < file.log.super_block.blocksPerSegment) {
            addrw = file.log.GetLogAddress(*(segments + i), j);
            
            std::cout << "CLEANER: Merge Segments LOOP (i,j) = " <<i << ","<< j << std::endl;

            /* Find next free slot */
            if (!found_free_slot) {
                while (*(segments + iw) != 0) {
                    while (jw < file.log.super_block.blocksPerSegment) {
                        addrw = file.log.GetLogAddress(*(segments + iw), jw);
                        br = file.log.GetBlockUsage(addrw);
                        if (br.use == static_cast<char>(usage::FREE)) {
                            found_free_slot = true;
                            break;
                        }
                        jw++;
                    }
                    if(found_free_slot) 
                        break;
                    jw = file.log.SummaryBlockSize();
                    iw++;
                }
            }
           
            /* check if we found a block to move */
            move = file.log.GetBlockUsage(addrr);

            /* move a block if so */
            if (move.use == static_cast<char>(usage::INUSE) && found_free_slot == true) {

                /* Update usage block */
                file.log.Read(addrr, file.log.super_block.bytesPerBlock, move_buf);
                file.log.Write(addrw, file.log.super_block.bytesPerBlock, move_buf);
                file.log.SetBlockUsage(addrw, br);
                file.log.ResetBlockUsage(addrr);

                /* Update inode pointer */
                Inode in = GetInode(move.inum);
                UpdateInode(in, addrr, addrw);
	            file.fileWrite(&file.log.iFile, in.inum * sizeof(Inode), sizeof(Inode), &in);

                found_free_slot = false;
            }
            j++;
        }
        j = file.log.SummaryBlockSize();
        i++;
    }
    
    file.log.super_block.usedSegments--;  
    std::cout << "CLEANER_MERGE_SEGMENTS END" << std::endl;

    return true;
}


bool Cleaner::CleanBlocks()
{
    std::cout << "CleanBlocks BEGIN" << std::endl;
    int total = 0;
    //TODO switch from calloc to some other alternative
    int *freeBlocks = (int*)calloc(sizeof(int), file.log.super_block.sectorCount+2);
    int j = 1, i = 0;
    
    for (; i <= file.log.super_block.segmentCount; i++) {
        int local_total = file.log.GetFreeBlockCount(i);
            
        /* Note too full, can merge */
        if ( local_total > 0 ) {
            total += local_total;
            *(freeBlocks+j) = i;
            j++;
        }

        std::cout << "FREE COUNT TOTAL: " << total << std::endl;

        /* Check if we have found enough free blocks to do merging */
        if (total > file.log.super_block.blocksPerSegment && i > 1) {
            *(freeBlocks+j) = 0;
            break;
        }
    }
    
    std::cout << "CLEANER NUMBER TO MERGE: " << i << std::endl;

    MergeSegments(freeBlocks);
    
    std::cout << "CleanBlocks END" << std::endl;

    free(freeBlocks);

    return true;
}

/**
 * Perform the clean operation.
 */
bool Cleaner::Clean()
{
	
    std::cout << "CLEANER_CLEAN BEGIN" <<std::endl;
    std::cout <<"CLEANER USED SEGS: " << file.log.super_block.usedSegments << std::endl;
    std::cout <<"CLEANER TOTAL SEGS: " << file.log.super_block.segmentCount << std::endl;
    std::cout <<"CLEANER START: " << state.startCleaner << std::endl;
    std::cout <<"CLEANER STOP: "<< state.stopCleaner << std::endl;

    bool rv = true;

    if(state.startCleaner >= (file.log.super_block.segmentCount - file.log.super_block.usedSegments)) {
        int blocksToFree = state.stopCleaner - state.startCleaner;
        std::cout << "CLEANER will clean: " << blocksToFree << std::endl;
        while(blocksToFree > 0) {
            bool didClean = CleanBlocks();
            if(!didClean) {
                std::cout << "CLEANER DID NOT CLEAN BLOCK!!!!" << std::endl;
                rv = false;
                break;
            }
            blocksToFree--;
        }
    } else {
        std :: cout << "Clean() RETURNS TRUE" << std::endl;
    }

    //TODO checkpoint?
    return rv;
}
