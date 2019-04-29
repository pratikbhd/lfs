#include <iostream>
#include "file.hpp"

void File::getiFile() {
    char buffer[FLASH_SECTOR_SIZE+2];
    iFile = Inode();
    int res = Flash_Read(log.flash, LOG_IFILE_OFFSET, 1, buffer);
    if (res) {
        std::cout <<"[File_Private] iFile READ FAIL" << std::endl;
    }
    std::memcpy(&iFile, buffer, sizeof(Inode));
}

log_address File::getNewLogEnd(){
    block_usage b = {0,static_cast<char>(usage::INUSE), std::time(nullptr)};
    log_address finder = log.Log_end_address;

    while (b.use == static_cast<char>(usage::INUSE)) {
        finder = log.GetNextFreeBlock(finder);
        //Getting a block in a new segment will force flushing the current segment to flash.
        //Thus it is not necessary to flush a segment again in checkpoint, 
        //But we still do it as of now to maintain semantics of the checkpoint operation.
        b = log.GetBlockUsage(finder); 
    }

    if (log.Log_end_address.segmentNumber != finder.segmentNumber){
        segments_done++;
        log.super_block.usedSegments++;
    }

    log.Log_end_address = finder;
    checkpoint();    
    return log.Log_end_address;
}

bool File::updateInode(Inode *i, int index, log_address address) {
    block_usage b;
    b.inum = (*i).inum;
    b.use = static_cast<char>(usage::INUSE);
    b.age = std::time(nullptr);

    if(index < 0) {
        return false;
    } else if(index < 4) {
        if((*i).block_pointers[index].segmentNumber > 0) {
            log.ResetBlockUsage((*i).block_pointers[index]);
        }
        (*i).block_pointers[index] = address;
    } else {
        char data[log.super_block.bytesPerBlock];
        unsigned int offsetBytes = (index - 4) * sizeof(log_address); //(ending index-4 added to account for \0)
        if (offsetBytes + sizeof(log_address) + 1 > log.super_block.bytesPerBlock)
            throw "Log::UpdateInode - the indirect block pointer data exceeded a block size. Not supported as of now.";

        if((*i).indirect_block.segmentNumber > 0) {
            log.Read((*i).indirect_block, log.super_block.bytesPerBlock, data);

            log.ResetBlockUsage((*i).indirect_block);
            (*i).indirect_block = getNewLogEnd();

            log_address old = {0,0};
            memcpy(&old, data+offsetBytes, sizeof(log_address)+1);
            log.ResetBlockUsage(old);
        } else {
            (*i).indirect_block = getNewLogEnd();
            memset(data, 0, sizeof(log.super_block.bytesPerBlock));
            unsigned int init_offset = 0;
            log_address default_address = {0,0};
            while(init_offset + sizeof(log_address) < log.super_block.bytesPerBlock){
                memcpy((data + init_offset), &default_address, sizeof(log_address));
                init_offset += sizeof(log_address);
            }
        }

        memcpy((data + offsetBytes), &address, sizeof(log_address));
        //write back indirect block at the log end.
        log.Write((*i).indirect_block, log.super_block.bytesPerBlock, data);
        //update block usage of indirect block at log end.
        log.SetBlockUsage((*i).indirect_block, b);
    }

    //update block usage of new block pointer of inode in flash
    log.SetBlockUsage(address, b);
    return true;
}

bool File::write(Inode *target, unsigned int blockNumber, int length, const char* buffer) {
    int max_block_pointers = 4 + (log.super_block.bytesPerBlock/sizeof(log_address));
    
    if (length > ((log.super_block.bytesPerBlock)*max_block_pointers)) {
        throw "[File_Private] Log::Write() - File length exceeds the maximum permitted file size!";
    }

	std::cout << "[File_Private] Write Enter ==> length: "<< length <<", file size: "<< (*target).fileSize << ", in->inum: " << (*target).inum << std::endl;

    Inode *update = target;

    if ((*target).inum == static_cast<unsigned int>(reserved_inum::IFILE)) {
        update = &iFile;
    }

    /* Do the write one block at a time.*/
	int i = 0; // Buffer offset, number of blocks
    while (length > 0) {
        if (blockNumber > max_block_pointers) throw "[File_Private] Log::Write() - File block numbers exceeds the maximum permitted file size!";

        // get new log end address
        log_address address = getNewLogEnd();
  
        // get old block pointer data
        char oldData[log.super_block.bytesPerBlock];
        if(GetLogAddress(*update, blockNumber).segmentNumber != 0) {
            log_address lax = GetLogAddress(*update, blockNumber);
            log.Read(lax, log.super_block.bytesPerBlock, oldData);
        }
        
        // add new data to old block pinter data
		memcpy(oldData, buffer + (log.super_block.bytesPerBlock*i), log.super_block.bytesPerBlock);

        // update new block with updated data
        log.Write(address, log.super_block.bytesPerBlock, oldData);

        // Set usage. Should this be done twice to make sure?
        updateInode(update, blockNumber, address);
        std::cout << "[File_Private] Write Block (input address params)==> blocknum: " << blockNumber << " sn: " << address.segmentNumber << " blk_offset: " << address.blockOffset;
        
        log_address verify = GetLogAddress(*update, blockNumber);
        std::cout << "[File_Private] Write Block (updated address params)==> blocknum: " << blockNumber << " sn: " << verify.segmentNumber << " blk_offset: " << verify.blockOffset;

        // one block written successfully.
        length -= log.super_block.bytesPerBlock;

        //increment blocknum
        blockNumber++;
		i++;
    }
	
    /* Update Inode in ifile, unless this is the ifile*/
    if ((*target).inum != static_cast<unsigned int>(reserved_inum::IFILE)) {
        int bytes_left = sizeof(Inode);
        int iFile_block_number     = 0; 
    	
        // Set ifile size	
	    if ((1+update->inum) * sizeof(Inode) > iFile.fileSize)
             iFile.fileSize = (1+update->inum) * sizeof(Inode);
        
        bool first_iter = true;
        while(bytes_left > 0) {
            char buffer[log.super_block.bytesPerBlock+1];

            //bytes which will be written successfully in one iteration.
            int bytes_done     = 0;
            int offset = 0;
            int begin = (update->inum * sizeof(Inode));
            int end   = begin + sizeof(Inode);
            int split_point = 0;
            
            std::cout << "[File_Private] Write iFile INUM: " << update->inum << " BEGIN: " << begin << " END: " << end << std::endl;

            //The iFile is a non continuous set of blocks identified by block number we try to maximize the usage of these blocks, if inode doesnt fit perfectly.
            if (begin % log.super_block.bytesPerBlock > end % log.super_block.bytesPerBlock) {
                //The inode will be split across two blocks of the iFile. Inode wont be split across more than two.
                if (first_iter) {
                    //Get the block number in iFile corresponding to the inum.
                    iFile_block_number = begin / log.super_block.bytesPerBlock;
                    //offset to first byte of the inode in the iFile block having current inum.
                    offset = begin % log.super_block.bytesPerBlock;
                    bytes_done = log.super_block.bytesPerBlock - offset;
                } else {
                    //second half of the inode to be written to the second block.
                    iFile_block_number = iFile_block_number + 1;
                    offset    = 0;
                    bytes_done   = (end % log.super_block.bytesPerBlock);
                    split_point = sizeof(Inode) - bytes_done;
                }
            } else {
                //The entire inode fits in one block of the iFile.
                iFile_block_number = begin / log.super_block.bytesPerBlock;
                offset    = begin % log.super_block.bytesPerBlock;
                bytes_done = sizeof(Inode);
            }

            std::cout << "[File_Private] Write iFile split_point: " << split_point << " BLOCKNUM: " << iFile_block_number << " offset: " << offset << " bytes_done: "<< bytes_done << std::endl;

            bool new_ifile_block = false;
            if(GetLogAddress(iFile, iFile_block_number).segmentNumber == 0) {
                updateInode(&iFile, iFile_block_number, getNewLogEnd());
                new_ifile_block = true;
            }

            log_address address = GetLogAddress(iFile, iFile_block_number);
            //Read one block of the iFile.
            log.Read(address, log.super_block.bytesPerBlock, buffer);
            //update the block at the location with the inode, considering split point, if the inode was split.
            memcpy((buffer+offset), (((char*)update)+split_point), bytes_done);

            //write the updated block of the ifile to the log end.
            if(!new_ifile_block) {
                address = getNewLogEnd();
                updateInode(&iFile, iFile_block_number, address);
            }
            log.Write(address, log.super_block.bytesPerBlock, buffer);

            bytes_left -= bytes_done;
            first_iter = false;
        }
    }

    //run segment cleaner
    clean();

    return true;
}

void File::checkpoint() {
    if(segments_done < max_interval) {
        return;
    }

    Checkpoint current, other;
    bool first;
    if (log.cp1.time < log.cp2.time) {
        current = log.cp1;
        other = log.cp2;
        first = true;
    } else {
        current = log.cp2;
        other = log.cp1;
        first = false;
    }
    current.address = log.Log_end_address;
    current.time = other.time + 1;

    /* Flush log end to flash */
    (*log.log_end).Flush();
    (*log.log_end).Load((*log.log_end).GetSegmentNumber());

    /* Erase first segment for flash metadata */
    int res = Flash_Erase(log.flash, 0, 1);
    if (res) {
        std::cout << "[File_Private] Erasing superblock on flash failed"; 
        return;
    }
    
    res = Flash_Write(log.flash, LOG_IFILE_OFFSET, 1, &iFile);
    if (res) {
        std::cout << "[File_Private] iFile write failed";
        return;
    }

    res = Flash_Write(log.flash, LOG_SUPERBLOCK_OFFSET, 1, &log.super_block);
    if (res) {
        std::cout << "[File_Private] superblock write failed";
        return;
    }

    res = Flash_Write(log.flash, first ? LOG_CP1_OFFSET : LOG_CP2_OFFSET, 1, &current);
    if (res) {
        std::cout << "[File_Private] checkpoint write failed";
        return;
    }

    segments_done = 0;
}