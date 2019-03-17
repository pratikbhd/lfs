#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <iostream>
#include <sys/stat.h>

#include "file.hpp"
#include "log.h"
#include "segment.h"
#include "directory.hpp"

#define MAX(x,y) (x < y ? y : x)
#define MIN(x,y) (x > y ? y : x)

char convertMode(mode_t mode);

File::File(char* lfsFile) {
    log = Log();
    unsigned int blocks;
    log.flash = Flash_Open(lfsFile, 0, &blocks); // TODO: Get the filename in state->lfsfile (char* format)
    log.GetSuperBlock();
    log.InitializeCache();
    log.cp1 = log.GetCheckpoint(LOG_CP1_OFFSET);
    log.cp2 = log.GetCheckpoint(LOG_CP2_OFFSET);
    log.GetiFile();
}

/**
 * Write <length> bytes of the file associated with <inode>, starting at <offset>.
 * The data written is taken from <buf>.
 */
int File::fileWrite(Inode *inode, int offset, int length, const void *buffer) {
    if (offset + length > GetMaxFileSize()) {
		std::cout << "fileWrite: offset" << offset << "length" << length << "greater than max size" << GetMaxFileSize() << std::endl;
        return -1;
    }
    
    // Read whole file
    std::cout << "MSIZE: " << log.super_block.bytesPerBlock*4 << std::endl;
    std::cout << "->size: " << GetMaxFileSize << std::endl;

    char data[GetMaxFileSize()];
    int writeIndex=0, writeOffset=0;

    while (log.GetLogAddress(*inode, writeIndex).segmentNumber != 0 ) { //BLOCK_NULL_ADDR) {
        std::cout << "->woff:" << writeOffset << std::endl;
        std::cout << "bytesPerBlock" << log.super_block.bytesPerBlock << std::endl;

        log.Read(log.GetLogAddress(*inode, writeIndex), log.super_block.bytesPerBlock, data+writeOffset);
        writeOffset+=log.super_block.bytesPerBlock;
        writeIndex++;

	}
    
   	if (offset > inode->fileSize) { // Fill holes
		memset(data+inode->fileSize, 0, offset - inode->fileSize);
	}	
    
    // write
    std::cout << "wh:" << data << std::endl;
    std::cout << "wo:" << data+offset << std::endl;
    std::cout << "bu:" << buffer << std::endl;
    std::cout << "le:" << length << std::endl;
    std::cout << "of:" << offset << std::endl;

    memcpy((data+offset), buffer, length);
   
	// Update size
    if(offset + length > inode->fileSize)
        inode->fileSize = offset + length;    

    // Just debugging
	int idx = 0;
	if (data[0] == '\0') {
		std::cout << "First char is nul\n" << std::endl;
    }
    for (idx = 0; idx < inode->fileSize; idx++) {
        if (idx < inode->fileSize) {
            std::cout << "eval\n" << std::endl;
            break;
        }
        std::cout << "loop i: %d\n" << idx << std::endl;
        std::cout << data[idx] << std::endl;
    }
    std::cout << "after\n" << std::endl;

	// Write entire file	
	log.Write(inode,
			offset / (log.super_block.bytesPerBlock),
			length + (offset % log.super_block.bytesPerBlock), // Write all data including the beginning of first block
			data + offset - (offset % (log.super_block.bytesPerBlock))
			);

    return length;
}

/**
 * Reads <length> bytes from file <inode> beginning at <offset>, then stores them
 * in <buffer>. 
 */ 
int File::fileRead(Inode *inode, int offset, int length, void *buffer) {
	int total;
	
    std::cout << "FileRead: Reading inum: " << inode->inum << std::endl;

    // If the length to be read in is greater than the maximum file size, set the length to the maximum file size
    if (length > GetMaxFileSize()) {
		std::cout << "FileRead: length" << length << "greater than max: " << GetMaxFileSize() << std::endl;
		length = GetMaxFileSize();
    }

    // If the part of data to be read exceeds the file size of the data in the inode, then truncate the read length only upto the end of the file
   	if (length + offset > inode->fileSize) {
		std::cout << "fileRead read" << length << "bytes at offset" << offset << "total =" << length+offset << std::endl;
		std::cout << "inode file size: " << inode->fileSize << std::endl;
		std::cout << "Bad line fileRead, read past EOF\n" << std::endl;
		// Truncate read length
		length = inode->fileSize - offset;
	}

    // Finally, set the total bytes to be read in to the variable total
	total = length;

	char block[log.super_block.bytesPerBlock]
	  , *bufferPosition = static_cast<char*>(buffer);

	int readCount = 0
	  , remainingBlockPart = offset % log.super_block.bytesPerBlock
	  , index  = offset / log.super_block.bytesPerBlock;
      
	log_address address;

    // First, read the first partial block 
	if (remainingBlockPart != 0) {
		address = log.GetLogAddress(*inode, index);
		log.Read(address, log.super_block.bytesPerBlock, block);
		
        // Copy to bufferPosition the contents of block after the offset, but at most length bytes
		readCount = (length > log.super_block.bytesPerBlock-remainingBlockPart ? log.super_block.bytesPerBlock-remainingBlockPart : length);
		memcpy(bufferPosition, block + remainingBlockPart, readCount);

		bufferPosition += readCount;
		length -= readCount;
		index++;
	}
	
    // Read the remaining blocks, including the last partial block
	while (length > 0) {
		address = log.GetLogAddress(*inode, index);
		readCount = (length > log.super_block.bytesPerBlock-remainingBlockPart ? log.super_block.bytesPerBlock-remainingBlockPart : length); // Read a whole block or whatever remains
		log.Read(address, readCount, block);
		memcpy(bufferPosition, block, readCount);

		bufferPosition += readCount;
		length -= readCount;
		index++;
	}

	// Return the amount read
	return total;
}


int fileCreate(const char *path, mode_t mode, struct fuse_file_info *fi) {
	std::cout << "FileCreate: path " << path << std::endl;
    Inode inode, dir;

    int length = 0, returnval = 0;
	char *name , *parent; 
	SplitPathAtEnd(path, &parent, &name);
	cout << "asds";
	if (path[0] != '/') {
		DEBUG(("FileCreate: path %s didn't start with '/'\n", path));
		returnval = -ENOENT; // Must be at root directory
		goto done;
	}

	length = strlen(name);
    if (length > NAME_MAX_LENGTH) {
		DEBUG(("FileCreate: Name %s is too long\n", name));
        returnval = ENAMETOOLONG; // path too long
		goto done;
    }
    // Parse path path to find inode
    int err = Directory_ReadPath(path, &inode);

    if (err) { // No inode found
		// First make an inode
		if (Directory_NewInode(&inode) == -EFBIG) {
			DEBUG(("FileCreate: Cannot allocate new inode, ifile too large\n"));
			returnval = -EFBIG;
			goto done;
		}
		DEBUG(("FileCreate: Made new inode %d\n", inode.inum));
		Directory_ToggleInumIsUsed(inode.inum);

//		for (i = 0; i < length; i++)
//			inode.name[i] = name[i];
//		inode.name[i] = '\0';

		inode.fileSize = 0;
		inode.fileType = convertMode(mode);
		DEBUG(("FileCreate: Mode = %o\n", mode));
		DEBUG(("FileCreate: inum = %d\n", inode.inum));

		inode.hardLinkCount = 1;
		length = strlen(path);

		err = Directory_ReadPath(parent, &dir);
		if (err) {
			DEBUG(("FileCreate: Error %s reading parent directory %s\n", strerror(err), parent));
			returnval = err;
			goto done;
		}
		err = Directory_AddEntry(&dir, &inode, name);
		if (err) {
			DEBUG(("FileCreate: Error %s adding entry %s in directory %s\n", strerror(err), name, parent));
			returnval = err;
			goto done;
		}
		File_Write(&inode, 0, 0, NULL);
	//	File_Write(ifile, sizeof(Inode) * inode->inum, sizeof(Inode), inode);
		/*
		// Inodes per Block = BLOCK_SIZE / sizeof(Inode)
		block = inode->inum / (BLOCK_SIZE / sizeof(Inode)) + inode->inum % (BLOCK_SIZE / sizeof(Inode));
		// Write new inode. File is empty, so don't write that yet.
		log_write_inode(ifile, block, sizeof(*inode), (char *) inode); 
		*/
    } 
	DEBUG(("FileCreate: Returning from creating \"%s\"\n", path));
	printInode(&inode);
	returnval = File_Open(path, fi);
done:
	debug_free(parent);
	debug_free(name);
    return returnval;
    
}

unsigned int File::GetMaxFileSize() {
	return (log.super_block.bytesPerBlock * (4 + (log.super_block.bytesPerBlock)/sizeof(log_address)));
}

// DONE
// fileWrite -> file_write
// fileRead -> file_read

//TODO
// fileOpen -> 
// fileCreate -> 