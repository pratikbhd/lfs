#include <iostream>
#include "lfs_enums.h"
#include "log.h"
#include <string>

void Log::GetSuperBlock() {
    char buffer[FLASH_SECTOR_SIZE+2];
    super_block = SuperBlock();
    int res = Flash_Read(flash, LOG_SUPERBLOCK_OFFSET, 1, buffer);
    if (res) {
        std::cout <<"SuperBlock READ FAIL" << std::endl;
    }
    std::memcpy(&super_block, buffer, sizeof(SuperBlock)+1);
}

void Log::GetiFile() {
    char buffer[FLASH_SECTOR_SIZE+2];
    iFile = Inode();
    int res = Flash_Read(flash, LOG_IFILE_OFFSET, 1, buffer);
    if (res) {
        std::cout <<"iFile READ FAIL" << std::endl;
    }
    std::memcpy(&iFile, buffer, sizeof(Inode));
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
        throw "Log::Read() - Cannot read more than a segment! - " + std::to_string(offsetBytes + length);

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
        throw "Log::Write() - Cannot write more than a segment! - " + std::to_string(offsetBytes + length);

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
        throw "Log::Free() - block offset is out of bounds of segment! - " + std::to_string(address.blockOffset);

    resetBlockUsage(address);

    /* blocks to be freed should be loaded to main memory */
    if((*log_end).GetSegmentNumber() != address.segmentNumber) {
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

log_address Log::GetLogAddress(Inode i, int index) {
    log_address address = {0, 0};

    if(index < 0) {
        throw "Log::GetLogAddress() - Invalid index for block pointer. Index is negative.";
    } else if(index < 4) {
        if(i.block_pointers[index].segmentNumber > 0) {
            return i.block_pointers[index];
        }
    } else {
        if(i.indirect_block.segmentNumber > 0) {
            char data[super_block.bytesPerBlock];
            unsigned int offsetBytes = (index - 4) * sizeof(log_address);
            if (offsetBytes + sizeof(log_address) > super_block.bytesPerBlock)
                throw "Log::GetLogAddress() - the index specified references block pointer data that exceeds a block size. Not supported as of now.";

            Read(i.indirect_block, super_block.bytesPerBlock, data);
            memcpy(&address, (data + offsetBytes), sizeof(log_address));
        }
    }
    return address;
}

bool Log::Write(Inode *target, unsigned int blockNumber, int length, const char* buffer) {
    int max_block_pointers = 4 + (super_block.bytesPerBlock/sizeof(log_address));
    
    if (length > ((super_block.bytesPerBlock)*max_block_pointers)) {
        throw "Log::Write() - File length exceeds the maximum permitted file size!";
    }

	std::cout << "Write Enter ==> length: "<< length <<", file size: "<< (*target).fileSize << ", in->inum: " << (*target).inum << std::endl;

    Inode *update = target;

    if ((*target).inum == static_cast<unsigned int>(reserved_inum::IFILE)) {
        update = &iFile;
    }

    /* Do the write one block at a time.*/
	int i = 0; // Buffer offset, number of blocks
    while (length > 0) {
        if (blockNumber > max_block_pointers) throw "Log::Write() - File block numbers exceeds the maximum permitted file size!";

        // get new log end address
        log_address address = getNewLogEnd();
  
        // get old block pointer data
        char oldData[super_block.bytesPerBlock];
        if(GetLogAddress(*update, blockNumber).segmentNumber != 0) {
            log_address lax = GetLogAddress(*update, blockNumber);
            Read(lax, super_block.bytesPerBlock, oldData);
        }
        
        // add new data to old block pinter data
		memcpy(oldData, buffer + (super_block.bytesPerBlock*i), super_block.bytesPerBlock);

        // update new block with updated data
        Write(address, super_block.bytesPerBlock, oldData);

        // Set usage. Should this be done twice to make sure?
        UpdateInode(update, blockNumber, address);
        std::cout << "Write Block (input address params)==> blocknum: " << blockNumber << " sn: " << address.segmentNumber << " blk_offset: " << address.blockOffset;
        
        log_address verify = GetLogAddress(*update, blockNumber);
        std::cout << "Write Block (updated address params)==> blocknum: " << blockNumber << " sn: " << verify.segmentNumber << " blk_offset: " << verify.blockOffset;

        // one block written successfully.
        length -= super_block.bytesPerBlock;

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
            char buffer[super_block.bytesPerBlock+1];

            //bytes which will be written successfully in one iteration.
            int bytes_done     = 0;
            int offset = 0;
            int begin = (update->inum * sizeof(Inode));
            int end   = begin + sizeof(Inode);
            int split_point = 0;
            
            std::cout << "Write iFile INUM: " << update->inum << " BEGIN: " << begin << " END: " << end << std::endl;

            //The iFile is a non continuous set of blocks identified by block number we try to maximize the usage of these blocks, if inode doesnt fit perfectly.
            if (begin % super_block.bytesPerBlock > end % super_block.bytesPerBlock) {
                //The inode will be split across two blocks of the iFile. Inode wont be split across more than two.
                if (first_iter) {
                    //Get the block number in iFile corresponding to the inum.
                    iFile_block_number = begin / super_block.bytesPerBlock;
                    //offset to first byte of the inode in the iFile block having current inum.
                    offset = begin % super_block.bytesPerBlock;
                    bytes_done = super_block.bytesPerBlock - offset;
                } else {
                    //second half of the inode to be written to the second block.
                    iFile_block_number = iFile_block_number + 1;
                    offset    = 0;
                    bytes_done   = (end % super_block.bytesPerBlock);
                    split_point = sizeof(Inode) - bytes_done;
                }
            } else {
                //The entire inode fits in one block of the iFile.
                iFile_block_number = begin / super_block.bytesPerBlock;
                offset    = begin % super_block.bytesPerBlock;
                bytes_done = sizeof(Inode);
            }

            std::cout << "Write iFile split_point: " << split_point << " BLOCKNUM: " << iFile_block_number << " offset: " << offset << " bytes_done: "<< bytes_done << std::endl;

            bool new_ifile_block = false;
            if(GetLogAddress(iFile, iFile_block_number).segmentNumber == 0) {
                UpdateInode(&iFile, iFile_block_number, getNewLogEnd());
                new_ifile_block = true;
            }

            log_address address = GetLogAddress(iFile, iFile_block_number);
            //Read one block of the iFile.
            Read(address, super_block.bytesPerBlock, buffer);
            //update the block at the location with the inode, considering split point, if the inode was split.
            memcpy((buffer+offset), (((char*)update)+split_point), bytes_done);

            //write the updated block of the ifile to the log end.
            if(!new_ifile_block) {
                address = getNewLogEnd();
                UpdateInode(&iFile, iFile_block_number, address);
            }
            Write(address, super_block.bytesPerBlock, buffer);

            bytes_left -= bytes_done;
            first_iter = false;
        }
    }

    return true;
}


//Done:
//Internal:
//log_set_block_free -> resetBlockUsage()
//log_set_block -> setBlockUsage()
//log_write -> Write(log_address address, int length, char *buffer)
//log_get_higher_addr() -> getNextFreeBlock
//log_checkpoint_update() -> checkpoint()
//log_get_next_address() -> getNewLogEnd
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