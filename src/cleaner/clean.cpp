#include <iostream>
#include "cleaner.hpp"

Cleaner::Cleaner(char* lfsFile): file(File(lfsFile)) {
	
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
