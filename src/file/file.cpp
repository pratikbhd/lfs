#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <iostream>
#include<exception>
#include <string.h>
#include <sys/stat.h>

#include "file.hpp"
#include "log.h"
#include "segment.h"

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

int File::fileWrite(Inode *inode, int offset, int length, const void *buffer) {
    if (offset + length > GetMaxFileSize()) {
		std::cout << "fileWrite: offset" << offset << "length" << length << "greater than max size" << GetMaxFileSize() << std::endl;
        return -1;
    }
    
    // Read whole file
    std::cout << "Maximum Size: " << log.super_block.bytesPerBlock*4 << std::endl;
    std::cout << "size: " << GetMaxFileSize() << std::endl;

    char data[GetMaxFileSize()];
    int writeIndex=0, writeOffset=0;

    while (log.GetLogAddress(*inode, writeIndex).segmentNumber != 0 ) { //BLOCK_NULL_ADDR) {
        std::cout << "Write offset:" << writeOffset << std::endl;
        std::cout << "bytesPerBlock" << log.super_block.bytesPerBlock << std::endl;

        log.Read(log.GetLogAddress(*inode, writeIndex), log.super_block.bytesPerBlock, data+writeOffset);
        writeOffset+=log.super_block.bytesPerBlock;
        writeIndex++;
	}
    
   	if (offset > inode->fileSize) { // Fill holes
		memset(data+inode->fileSize, 0, offset - inode->fileSize);
	}	
    
    // write
    std::cout << "Data:" << data << std::endl;
    std::cout << "Data+offset:" << data+offset << std::endl;
    std::cout << "Buffer:" << buffer << std::endl;
    std::cout << "Length:" << length << std::endl;
    std::cout << "Offset:" << offset << std::endl;

    memcpy((data+offset), buffer, length);
   
	// Update size
    if(offset + length > inode->fileSize)
        inode->fileSize = offset + length;    

    // Just debugging
	int idx = 0;
	if (data[0] == '\0') {
		std::cout << "First character is null\n" << std::endl;
    }
    for (idx = 0; idx < inode->fileSize; idx++) {
        if (idx < inode->fileSize) {
            std::cout << "eval\n" << std::endl;
            break;
        }
        std::cout << "loop i: %d\n" << idx << std::endl;
        std::cout << data[idx] << std::endl;
    }

	// Write entire file	
	log.Write(inode,
			offset / (log.super_block.bytesPerBlock),
			length + (offset % log.super_block.bytesPerBlock), // Write all data including the beginning of first block
			data + offset - (offset % (log.super_block.bytesPerBlock))
			);

    return length;
}

int File::fileRead(Inode *inode, int offset, int length, char *buffer) {
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

	char block[log.super_block.bytesPerBlock];
	char *bufferPosition = buffer;

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

int File::fileCreate(const char *path, mode_t mode, struct fuse_file_info *fi) {
	std::cout << "FileCreate: path " << path << std::endl;
    Inode inode, directory;
	
    int i, j, length = strlen(path), returnValue = 0;

	// Extract the parent path of the outermost path. We start with 1 positions before the end to skip the terminating '/' if present
	// First, shift the value of i till we reach the end of the parent path
	for (i = length - 2; i >= 0 && path[i] != '/'; i--);

	if (i < 0) {
		std::cout << "fileCreate: <path>" << path << "does not begin with '/'" << std::endl;
	}

	char name[static_cast<unsigned int>(fileLength::LENGTH)+1] , parent[i+1];
	
	// We populate the parent string
	for (j = 0; j < i; j++) {
		parent[j] = path[j];
	}
	parent[j] = '\0';

	// When the parent in the root itself
	if (i == 0) {
		parent[0] = '/';
		parent[1] = '\0';
	}

	// If the path terminates with a '/', remove it
	if (path[length-1] == '/')
		length--;

	// Now, populate the child name
	j += 1;
	for (i = 0; j < length && i < static_cast<unsigned int>(fileLength::LENGTH); j++, i++) {
		name[i] = path[j];
	} name[i] = '\0';

	std::cout << "fileCreate: path" << path << "parent" <<  parent << "child" << name << std::endl;

	// The path should start with a '/'
	if (path[0] != '/') {
		std::cout << "FileCreate: path" << path << "didn't start with '/'" << std::endl;
		return -ENOENT;
	}

	length = strlen(name);
    if (length > static_cast<unsigned int>(fileLength::LENGTH)) {
		std::cout << "FileCreate: Name" << name << "is too long" << std::endl;
        return ENAMETOOLONG;
	}

    // Parse path to find inode
    int error = ReadPath(path, &inode);

	// If an error shows up, a file on this path does not exist.
    if (error) {
		
		// First, make an inode
		if (CreateInode(&inode) == -EFBIG) {
			std::cout << "FileCreate: Cannot allocate new inode, ifile is too large\n" << std::endl;
			returnValue = -EFBIG;
			return returnValue;
		}

		std::cout << "FileCreate: Made new inode" << inode.inum << std::endl;

		inode.fileSize = 0;

		// Find the fileType.
		// TODO: Implement Symlink as well
		if ((mode & S_IFMT) == S_IFREG) {
			inode.fileType = static_cast<char>(fileTypes::PLAIN_FILE);
		}
		else if ((mode & S_IFMT) == S_IFDIR) {
			inode.fileType = static_cast<char>(fileTypes::DIRECTORY);
		}
		else if ((mode & S_IFMT) == S_IFLNK){
			inode.fileType = static_cast<char>(fileTypes::SYM_LINK);
		}
		else {
			inode.fileType = static_cast<char>(fileTypes::NO_FILE);
		}
		
 		std::cout << "FileCreate: Mode =" << mode << std::endl;
		std::cout << "FileCreate: inum =" << inode.inum << std::endl;

		length = strlen(path);
		error = ReadPath(parent, &directory);

		// If an error shows up, it means that the parent directory could not be read in
		if (error) {
			std::cout << "FileCreate: Error" << strerror(error) << "reading parent directory" << parent << std::endl;
			return error;
		}

		// Need to keep an <name, inum> pair in the corresponding directory
		error = NewEntry(&directory, &inode, name);
		if (error) {
			std::cout << "FileCreate: Error" <<  strerror(error) << "adding entry" << name << "in directory" << parent << std::endl;
			return error;
		}
		fileWrite(&inode, 0, 0, NULL);
    } 
	
	std::cout << "FileCreate: Returning from creating" << path << std::endl;
	returnValue = fileOpen(path, fi);
    return returnValue;
}

int File::CreateInode(Inode *inode) {
    std::cout << "Creating a new inode" << std::endl;

	int	i, length;
	char buffer[GetMaxFileSize()];
	Inode *inodes, ifile = log.iFile;

	fileRead(&ifile, 0, ifile.fileSize, buffer);
	inodes = (Inode *) buffer;
	length = ifile.fileSize / sizeof(Inode);
	for (i = 0; i < length; i++) {
		// Check if any inode's filetype has been assigned to NO_FILE which suggests that it is a free inode
		if (inodes[i].fileType == static_cast<char>(fileTypes::NO_FILE) && inodes[i].inum != static_cast<unsigned int>(reserved_inum::NOINUM)) {
			std::cout << "New inode is" << i << std::endl;
			memcpy(inode, inodes + i, sizeof(Inode));
			return 0;
		}
	}
	
	// No free inode was found, check if a new inode can be assigned to the ifile without exceeding the maximum file size liimitation
	if (ifile.fileSize + sizeof(Inode) > GetMaxFileSize())
		return -EFBIG;

	// Allocate a new inode if there is enough space
	memset(inode, 0, sizeof(Inode));
	std::cout << "Made a new inode of length" << length << std::endl;
	inode->fileType = static_cast<char>(fileTypes::NO_FILE);
	inode->inum = length;
	std::cout << "CreateInode: Writing inode" << inode->inum << "to ifile" << std::endl;
	fileWrite(&ifile, ifile.fileSize, sizeof(Inode), inode);
	std::cout << "CreateInode: Finished writing inode" << inode->inum << std::endl;
	return 0;
}

int File::fileOpen(const char *path, struct fuse_file_info *fi) {
	std::cout << "FileOpen" << path << std::endl;
    Inode inode;
    int error = ReadPath(path, &inode);
	if (error) {
		std::cout << "FileOpen: Could not read path" << path << std::endl;
		return error;
	}
	std::cout << "FileOpen: Success!" << std::endl;
    return 0;
}

int File::ReadPath(const char *path, Inode *inode) {
	
	std::cout << "ReadPath: path" << path << std::endl;	
	
	char directory[GetMaxFileSize()], name[static_cast<unsigned int>(fileLength::LENGTH)+1];
	int inum, length, offset, i, error 
		, pathLength = strlen(path);

	// The path must start with '/'
    if (path[0] != '/') {
		std::cout << "ReadPath: Invalid path, begins with" << path[0] << std::endl;
		return -EINVAL;
	}
	
	Inode directoryInode = ReturnInode(static_cast<char>(reserved_inum::ROOT));
	
	// Check if the path is just the root
	if (path[1] == '\0') {
		std::cout << "Read path on directory" << path <<  std::endl;
		std::cout << "Root->inum =" <<  directoryInode.inum << ", length =" << directoryInode.fileSize << std::endl;
		memcpy(inode, &directoryInode, sizeof(Inode));
		return 0;
	}

	// Read directory for next part of path. Initially directory is the root
	offset = 1;
	length = fileRead(&directoryInode, 0, directoryInode.fileSize, directory);
	std::cout << "ReadPath: directory is of length" << length << std::endl;
	
	// Start debugging
	std::cout << "Contents: " << std::endl;
	char tmp = directory[length];
	directory[length] = '\0';
	std::cout << directory << std::endl;
	directory[length] = tmp;
	// End debugging

	while (offset < pathLength) {
		
		// Copy a name from the path
		for (i = 0; (path + offset)[i] != '/' && (path + offset)[i] != '\0'; i++) {
			name[i] = (path + offset)[i];
		} 
		// Complete the name by storing a '\0' at the end of the string. This is the inode to return
		name[i] = '\0';

		std::cout << "ReadPath: getting inode for" << name << std::endl;
		
		error = ReturnInodeFromBuffer(directory, length, name, &inum);
		
		if (error) {
			std::cout << "ReadPath: Could not follow path" <<  "Failed to read" << name << std::endl;
			return error;
		}

		directoryInode = ReturnInode(inum);
		
		// From the above for loop, the value of i will have reached the end of the name and should point to either '/' or '\0'
		if ((path + offset)[i] == '\0' || (path + offset)[i+1] == '\0') { 
			std::cout << "ReadPath: Returning inode" <<  directoryInode.inum << std::endl;
			memcpy(inode, &directoryInode, sizeof(Inode));
			return 0;
		}
		else if (directoryInode.fileType != static_cast<char>(fileTypes::DIRECTORY)) {
			std::cout << "ReadPath: Link" << name << "in path" << name << "not a directory" << std::endl;
			return -ENOTDIR;
		}

		// The next offset will be the next directory in the path. The value of i will put us at the end of the current directory.
		// i+1 is done to include the '/' between two directories in the path
		offset += i + 1;
		length = fileRead(&directoryInode, 0, directoryInode.fileSize, directory);
	}

	return -ENOENT;
}

Inode *File::ReturnInode(int inum) {
	Inode *inode,
		ifile = log.iFile;

	int length = fileRead(&ifile, inum*sizeof(Inode), sizeof(Inode), (char*)&inode);  
	std::cout << "GetInode: Read" << length << "bytes from ifile, inum =" << inum << "inode->inum =" << inode.inum << std::endl;
	return inode;
}

int File::ReturnInodeFromBuffer(const char *buf, int length, const char *name, int *inum) {
	int num, offset;
	char temporaryName[static_cast<unsigned int>(fileLength::LENGTH)+1];
	
	// Check if name does not exist
	if (name[0] == '\0') {
		std::cout << "Null name in 'ReturnInodeFromBuffer'" << std::endl;
		return -ENOENT;
	}

	temporaryName[0] = '\0';
	
	// Start searching for the name
	offset = 0;
	while (strcmp(name,temporaryName) != 0 && offset < length) {
		memcpy(&num, buf + offset, sizeof(int));
		offset += sizeof(int);
		strcpy(temporaryName, buf + offset);
		offset += strlen(temporaryName) + 1;
	}

	if (strcmp(name,temporaryName) == 0) {
		std::cout << "inodeByName: inum =" << num << std::endl;
		*inum = num;
		return 0;
	}
	
	// Nothing was found
	return -ENOENT;
}

/**
 * Inserts the pair <file->inum,fileName> into the directory associated with <dir>.
 */
int File::NewEntry(Inode *directoryInode, Inode *fileInode, const char *fileName) {
	
	std::cout << "Add a new entry" << fileName << std::endl;
	int length, i, inum, insertLocation, compare;
   
	length = strlen(fileName);

    std::cout << "length:" << length <<std::endl;
    std::cout << "dirfilesize:" <<  directoryInode->fileSize << std::endl;
    
	char buffer[directoryInode->fileSize];
    char name[static_cast<unsigned int>(fileLength::LENGTH)+1];

	std::cout << "AddEntry: length =" << length << std::endl;
	fileRead(directoryInode, 0, directoryInode->fileSize, buffer);  	

	std::cout << "AddEntry: Inserting, dir size =" <<  directoryInode->fileSize << std::endl;
	
	insertLocation = 0;
	
	while (insertLocation < directoryInode->fileSize) {
		memcpy(&inum, buffer + insertLocation, sizeof(int));
		strcpy(name, buffer + insertLocation + sizeof(int));

		compare = strcmp(fileName, name);
		if (compare < 0) {
			break; // Found place to insert next entry
		} else if (compare == 0) {
			std::cout << "AddEntry: Adding existant file" << fileName << std::endl;
			return -EEXIST;
		}

		insertLocation += sizeof(int) + strlen(name) + 1; // skip inum, name and null byte.
	
	}

	// Going to write everything in the file starting from the new entry
	char newBuffer[sizeof(int) + length + 1 + directoryInode->fileSize - insertLocation]; // Room for inum, fileName + null byte, and rest of file
	
	// Copy next entry into buf
	std::cout << "AddEntry: Adding inum" << fileInode->inum << "name" << fileName << std::endl;
	
	memcpy(newBuffer, &(fileInode->inum), sizeof(int));
	std::cout << "1st memcpy done" << std::endl;

	memcpy(newBuffer + sizeof(int), fileName, length+1);
	std::cout << "2nd memcpy done" << std::endl;

	// Copy rest of directory 
	memcpy(newBuffer + sizeof(int) + length + 1
		, buffer+insertLocation
		, directoryInode->fileSize - insertLocation);

	std::cout << "3rd memcpy done" << std::endl;

	// Changed this from file to dir
	fileWrite(directoryInode, insertLocation, directoryInode->fileSize - insertLocation + sizeof(int) + length + 1, newBuffer); 
	
	std::cout << "Added entry" << fileName << "Contents of buffer: inum =" << *((int*) newBuffer) << "and name =" << (newBuffer + sizeof(int)) << std::endl;
	return 0;	
}

unsigned int File::GetMaxFileSize() {
	return (log.super_block.bytesPerBlock * (4 + (log.super_block.bytesPerBlock)/sizeof(log_address)));
}


int File::fileGetattr(const char *path, struct stat *stbuf) {
	std::cout << "Getattr being called." << std::endl;
    Inode fileInode;
    int error = ReadPath(path, &fileInode);
	
	// Check if the path does not exist or is created
	if (error) {
		std::cout << "ENOENT in Getattr" << std::endl;
		return error;
	}

	// Initialize the structure 'stbuf' to 0
	memset(stbuf, 0, sizeof(*stbuf));

	// Check if the inode of the 'path' is set to some value i.e. directory or file
	if (fileInode.fileType == static_cast<unsigned int>(fileTypes::NO_FILE)) {
		std::cout << "NO_FILE in Getattr, inum =" << fileInode.inum << "path =" << path << std::endl;
		return -ENOENT;
	}

    std::cout << "Getattr: Finished readpath" << std::endl;

	// Start setting the values of the stbuf
	stbuf->st_size = fileInode.fileSize;
    stbuf->st_blksize = log.super_block.bytesPerBlock;
    stbuf->st_ino = fileInode.inum;

	// Check the file type and assign accordingly
	switch (fileInode.fileType) {
		case static_cast<unsigned int>(fileTypes::DIRECTORY):
		    stbuf->st_mode |= S_IFDIR;
			break;
		case static_cast<unsigned int>(fileTypes::PLAIN_FILE):
			stbuf->st_mode |= S_IFREG;
			break;
		case static_cast<unsigned int>(fileTypes::SYM_LINK):
			stbuf->st_mode |= S_IFLNK;
			break;
	}

	// Assign the file permissions
	// For now, full permissions are given to all user types
	stbuf->st_mode |= S_IRWXU;
	stbuf->st_mode |= S_IRWXG;
	stbuf->st_mode |= S_IRWXO;

	std::cout << "Leaving getAttr" << std::endl;
	return 0;
}