#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fuse.h>
#include <iostream>
#include <exception>
#include <limits.h>
#include "directory.hpp"
#include "log.h"
#include "lfs_enums.h"

Directory::Directory(char* lfsFile): file(File(lfsFile)) {
	
}

//This function initializes the directory structure
void* Directory::Initialize(struct fuse_conn_info *conn) {	
// void* Directory::Initialize() {	


	Inode iFile = file.log.iFile; // Inode of the Ifile
	Inode directoryInode; // Inode for the directory
	Inode inodeArray[iFile.fileSize]; // This is an array of inodes that keeps tracks of the inodes currently being used
	
	// The total number of inodes allocated in a block
	inodes_length = file.log.super_block.bytesPerBlock / sizeof(Inode);
	for (int i = 0; i < file.GetMaxFileSize() / sizeof(Inode); i++)
		file.inodes_used.push_back(false);

	// If the iFile is currently empty, this means a new directory has been made
	// So, create an inode for the directory
	if (iFile.fileSize == 0) { 
		// The current length of inode will be only 1 since we are creating an inode for the directory
		inodes_length = 1;
		file.ToggleInumUsage(static_cast<int>(reserved_inum::ROOT));
		Inode directoryInode = Inode();
		directoryInode.inum = static_cast<char>(reserved_inum::ROOT);
		directoryInode.fileType = static_cast<char>(fileTypes::DIRECTORY);
		directoryInode.fileSize = 0;
		file.fileWrite(&directoryInode, 0, 0, NULL);
		file.log.Flush();
	
	} else {
		file.fileRead(&iFile, 0, iFile.fileSize, (char *)&inodeArray);
		inodes_length = 0;
		int i;
		for (i = 0; i < iFile.fileSize/sizeof(Inode); i++) {
			if (inodeArray[i].fileType != static_cast<char>(fileTypes::NO_FILE)) {
				file.ToggleInumUsage(i);
				inodes_length++;
			}
		}
	}
	return 0;
}

int Directory::makeDirectory(const char *path, mode_t mode) {
	
	Inode directoryInode;

	// Try to read the file/directory specified by the path that is given in the makeDirectory argument
	int error = 0;
	error = file.ReadPath(path, &directoryInode);
	// If no error shows up, it means the file/directory in the path is already present. Throw an error in this case
	if (!error) {
		std::cout << "[Directory] MakeDirectory: File: " << path << "already exists" << std::endl;
		return -EEXIST;
	}

	// Create a file in the provided path
	error = file.fileCreate(path, S_IFDIR, NULL);
	
	// If an error shows up, there was a problem creating the file/directory on the path
	if (error) {
		std::cout << "[Directory] makeDirectory: Could not create dir" << path << std::endl;
		return error;
	}

	// Try to read the file/directory specified by our path again
	error = file.ReadPath(path, &directoryInode);

	// If an error shows up, it could not read the file/directory given in the path
	// This is an error because we just created one above
	if (error) {
		std::cout << "[Directory] makeDirectory: Directory" << path << "was not created" << std::endl;
	} 
	else {
		// The directory was created successfullly. Print it's debug information
		std::cout << "[Directory] makeDirectory" << std::endl;
		std::cout << "[Directory] inum = " << directoryInode.inum << std::endl;
		std::cout << "[Directory] type =" << (directoryInode.fileType == static_cast<char>(fileTypes::DIRECTORY) ? "Directory" : "File") << std::endl;
	}
	return 0;
}

int Directory::directoryRead(const char *path, char *buffer, size_t length, off_t offset,
                struct fuse_file_info *fi) {
    std::cout << "[Directory] DirectoryRead: length =" << length << "offset" << offset << std::endl;
    Inode inode;
    int error = file.ReadPath(path, &inode);
        
    if (error) {
            return error;
    }
		
    int val = file.fileRead(&inode, offset, length, buffer);
    return val;
}

int Directory::directoryWrite(const char *path, const char *buffer, size_t length, off_t offset,
                struct fuse_file_info *fi) {

				std::cout << "[Directory] DirectoryWrite" << std::endl;
        Inode inode;
        int error = file.ReadPath(path, &inode);
        
				if (error) {
            return error;
        }

        int val = file.fileWrite(&inode, offset, length, buffer);

        return val;
}

	int Directory::Exists(const char * path)
	{
		Inode dirInode;
		int error = file.ReadPath(path, &dirInode);
		if (error) {
			std::cout << "[Directory] Exists: Could not find path " << path << std::endl;
			return -ENOENT;
		}

		if (dirInode.inum == static_cast<unsigned int>(reserved_inum::NOINUM)){
			std::cout << "[Directory] Exists: No inum assigned for path: " << path << std::endl;
			return -ENOENT;
		}

		return 0;
	}

int Directory::directoryReaddir(const char *path, 
					  void *buf, 
					  fuse_fill_dir_t filler, 
       				  off_t offset, 
					  struct fuse_file_info *finfo) {

	int   i, length, count, nRead, inum;

	struct stat st;

  char buffer[static_cast<unsigned int>(fileLength::LENGTH)], 
				name[256];

	Inode dirInode;
	int error = file.ReadPath(path, &dirInode);
	
	if (error) {
		std::cout << "[Directory] Readdir: Could not find path " << path << std::endl;
		return error;
	} else if (dirInode.fileType != static_cast<char>(fileTypes::DIRECTORY)) {
		std::cout << "[Directory] Readdir: " << path << "is not a directory. Filetype is " << dirInode.fileType << std::endl;
		return -ENOTDIR;
	}
	
	st.st_ino = dirInode.inum;
	file.fileRead(&dirInode, 0, dirInode.fileSize, buffer);
  filler(buf, ".", &st, 0);
  filler(buf, "..", &st, 0);
	
	std::cout << "[Directory] Readdir: Filled buf with . .." << std::endl;
	count = 0;
	length = dirInode.fileSize;
	std::cout << "[Directory] Readdir: Dir " << path << "has size " << dirInode.fileSize << std::endl;
    
	
  for (i = 0; i < length; i += nRead) {
		innerReadDir(buffer + i, &inum, name, &nRead); 
		st.st_ino = inum;
		filler(buf, name, &st, 0);
		count++;
  	}
	
	std::cout << "[Directory] Readdir: Return, " << count << std::endl;
  return 0;
}

int Directory::innerReadDir(char *directory, int *inum, char *name, int *lengthRead) {
	
	std::cout << "[Directory] internalReadDir" << std::endl;
	char *startName = directory + sizeof(int);
	int i;
	
	memcpy(inum, directory, sizeof(int));
	*lengthRead = sizeof(int);
	std::cout << "[Directory] lengthRead starts " << *lengthRead << std::endl;
	
	for (i = 0; startName[i] != '\0'; i++, (*lengthRead)++) {
		std::cout << "[Directory] Iterate readDir, read char " << startName[i] << std::endl;
		name[i] = startName[i];
	}

	name[i] = '\0';
	(*lengthRead)++;
	
	std::cout << "[Directory] internalReadDir: Return, lengthRead = " << *lengthRead << std::endl;
	return 0;
}

// Counts all inodes in use, except for the ifile inode.
int Directory::CountInodes() {
	int count = 0;
	for (int i = 0; i < inodes_length; i++) {
			count += file.inodes_used[i] & 1;
	}
	return count;
}

int Directory::Truncate(const char *path, off_t size) {
	Inode inode;
	int err = file.ReadPath(path, &inode);
	if (err) {
		return err;
	}
	err = file.Truncate(&inode, size);
	return err;
}

int Directory::Statfs(const char *path, struct statvfs *stbuf) {

	//TODO right now just filling dummy values
	memset(stbuf, 0, sizeof(struct statvfs));
	stbuf->f_bsize = file.log.super_block.bytesPerBlock;
	stbuf->f_blocks = file.log.super_block.blockCount;
	stbuf->f_bfree = file.log.GetUsedBlockCount();
	stbuf->f_bavail = stbuf->f_bfree;
	stbuf->f_files = CountInodes();
	stbuf->f_ffree = stbuf->f_files - inodes_length;
	stbuf->f_namemax = 256;
  return 0;
}