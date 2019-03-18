#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <iostream>
#include <limits.h>
#include "directory.hpp"
#include "log.h"
#include "file.hpp"
#include "lfs_enums.h"

// #define INODES_PER_SEGMENT SEGMENT_SIZE/sizeof(Inode)

// Inode *alloc_inode();
// int internalReadDir(char *, int *, char *, int *);

Directory::Directory(char* lfsFile): file(File(lfsFile)) {
	
}

//This function initializes the directory structure
// void* Directory::Initialize(struct fuse_conn_info *conn) {	
void* Directory::Initialize() {	

    // Log layer initialization

	Inode iFile = file.log.iFile; // Inode of the Ifile
	Inode directoryInode; // Inode for the directory
	Inode inodeArray[iFile.fileSize]; // This is an array of inodes

	// The total number of inodes allocated in a block
	inodes_length = file.log.super_block.bytesPerBlock / sizeof(Inode);

	unsigned int maxFileSize = file.GetMaxFileSize();

	// This is an array of bit integers i.e. 1s and 0s that keep track of the inodes being used
	// TODO: Think of another way to do this
	// TODO: inodes_used[(maxFileSize/sizeof(Inode))/sizeof(char) + ((maxFileSize/sizeof(Inode) % sizeof(char)) == 0 ? 0 : 1)] = {};

	// If the iFile is currently empty, this means a new directory has been made
	// So, create an inode for the directory
	if (iFile.fileSize == 0) { 
		// The current length of inode will be only 1 since we are creating an inode for the directory
		inodes_length = 1;
		
		// Directory_ToggleUsedInum(static_cast<char>(reserved_inum::ROOT));
		
		Inode directoryInode = Inode();
		directoryInode.inum = static_cast<char>(reserved_inum::ROOT);
		directoryInode.fileType = static_cast<char>(fileTypes::DIRECTORY);
		directoryInode.fileSize = 0;
		file.fileWrite(&directoryInode, 0, 0, NULL);
		file.log.Flush();
	
	} else {
		file.fileRead(&iFile, 0, iFile.fileSize, &inodeArray);
		inodes_length = 0;
		int i;
		for (i = 0; i < iFile.fileSize/sizeof(Inode); i++) {
			if (inodeArray[i].fileType != static_cast<char>(fileTypes::NO_FILE)) {
				// Directory_ToggleUsedInum(i);
				inodes_length++;
			}
		}
	}
	return 0;
}

// int Directory::makeDirectory(const char *path, mode_t mode) {
	
// 	Inode dirInode;
// 	int err = Directory_ReadPath(path, &dirInode);	

// 	if (!err) {
// 		std::cout << "MakeDir: File" << path << "already exists" << std::endl;
// 		return -EEXIST;
// 	}

// 	err = file.fileCreate(path, S_IFDIR, NULL);
// 	if (err) {
// 		std::cout << "MakeDir: Could not create dir" << path << std::endl;
// 		return err;
// 	}

// 	err = Directory_ReadPath(path, &dir);
// 	if (err) {
// 		std::cout << "MakeDir: Directory" << path << "was not created" << std::endl;
// 	} 
// 	else {
// 		std::cout << "MakeDir" << std::endl;
// 		std::cout << "\tinum = " << dirInode.inum << std::endl;
// 		std::cout << "\ttype =" << (dir.fileType == DIRECTORY ? "Directory" : "File") << std::endl;
// 	}
// 	return 0;
// }

// /**
//  * Stores an inode for the file specified by path in the inode parameter, or NULL if no such file exists.
//  * In that case, -ENOENT is returned, otherwise 0. Space is allocated for <inode> and must be freed by
//  * the caller.
//  */
// int Directory::ReadPath(const char *path, Inode *inode) {
	
// 	std::cout << "ReadPath: path" << path << std::endl;	
	
// 	char directory[GetMaxFileSize()],
// 		 name[static_cast<char>(fileLength::LENGTH)+1];
// 	int inum, length, offset, i, err 
// 		, pathLen = strlen(path);

// 	// The path must start with '/'
//     if (path[0] != '/') {
// 		std::cout << "ReadPath: Invalid path, begins with" << path[0] << std::endl;
// 		return -EINVAL;
// 	}
	
// 	Inode *dirIno = GetInode(static_cast<char>(reserved_inum::ROOT));
	
// 	if (path[1] == '\0') { // Just reading the dirIno
// 		std::cout << "Read path on directory" << path <<  std::endl;
// 		std::cout << "Root->inum =" <<  dirIno->inum << ", length =" << dirIno->fileSize << std::endl;
// 		memcpy(inode, dirIno, sizeof(Inode));
// 		return 0;
// 	}

// 	// Read directory for next part of path. Initially directory is the root
// 	offset = 1;
// 	length = file.Read(dirIno, 0, dirIno->fileSize, directory);
// 	std::cout << "ReadPath: dir is length" << length << std::endl;
	
// 	// Start debugging
// 	std::cout << "Contents: " << std::endl;
// 	char tmp = directory[length];
// 	directory[length] = '\0';
// 	std::cout << directory << std::endl;
// 	directory[length] = tmp;
// 	// End debugging

// 	while (offset < pathLen) {
// 		for (i = 0; (path + offset)[i] != '/' && (path + offset)[i] != '\0'; i++) { // Copy a name from the path
// 			name[i] = (path + offset)[i];
// 		} 
// 		name[i] = '\0'; // If (path + offset)[i] or next char is null, then end of path. This is inode to return
// 		std::cout << "ReadPath: getting inode for" << name << std::endl;
// 		err = inodeByNameInDirBuf(directory, length, name, &inum);
// 		debug_free(dirIno); // TODO: FREEE
// 		if (err) {
// 			std::cout << "ReadPath: Could not follow path" <<  "Failed to read" << name << std::endl;
// 			return err;
// 		}
// 		dirIno = GetInode(inum);
// 		if ((path + offset)[i] == '\0' || (path + offset)[i+1] == '\0') { // Found it, last char may be '/'
// 			std::cout << "ReadPath: Returning inode" <<  dirIno->inum) <<std::endl;
// 			memcpy(inode, dirIno, sizeof(Inode));
// 			debug_free(dirIno); // TODO: FREEEs
// 			return 0;
// 		}
// 		else if (dirIno->fileType != DIRECTORY) {
// 			std::cout << "ReadPath: Link" << name << "in path" << name < "not a directory" << std::endl;
// 			debug_free(dirIno); // TODO: FREEEs
// 			return -ENOTDIR;
// 		}
// 		offset += i + 1; // Advance to next dir in path
// 		length = file.Read(dirIno, 0, dirIno->fileSize, directory);
// 	}

// 	debug_free(dirIno); // TODO: FREEE
// 	return -ENOENT;
// }

// unsigned int Directory::GetMaxFileSize() {
// 	return (log.super_block.bytesPerBlock * (4 + (log.super_block.bytesPerBlock)/sizeof(log_address)));
// }

// Inode *GetInode(int inum) {
// 	Inode *inode, *ifile;
// 	ifile = log_get_ifile_inode();
// 	int length = file.Read(ifile, inum*sizeof(Inode), sizeof(Inode), inode);  
// 	std::cout << "GetInode: Read" << length << "bytes from ifile, inum =" << inum << "and inode->inum =" <<  inode->inum << std::endl;
// 	// TODO: Free the ifile - debug_free(ifile);
// 	return inode;
// }

// int Directory::inodeByNameInDirBuf(const char *buf, int length, const char *name, int *inum) {
// 	int num, offset;
// 	char tmpName[static_cast<char>(fileLength::LENGTH)+1];
	
// 	if (name[0] == '\0') {
// 		std::cout << "Null name in 'inodeByNameInDirBuf'" << std::endl;
// 		return -ENOENT; // No name
// 	}

// 	tmpName[0] = '\0';
// 	// Search for name
// 	offset = 0;
// 	while (strcmp(name,tmpName) != 0 && offset < length) {
// 		memcpy(&num, buf + offset, sizeof(int));
// 		offset += sizeof(int);
// 		strcpy(tmpName, buf + offset);
// 		offset += strlen(tmpName) + 1;
// 	}

// 	if (strcmp(name,tmpName) == 0) {
// 		std::cout << "inodeByName: inum =" << num << std::endl;
// 		*inum = num;
// 		return 0;
// 	}
// 	// Not found
// 	return -ENOENT;
// }

// int Directory::SplitPathAtEnd(const char *path, char *prefix, char *child) {
// 	int length = strlen(path);
// 	int i, j;

// 	// i starts at 2nd to last character, to skip terminating '/' character if present
// 	for (i = length - 2; i >= 0 && path[i] != '/'; i--);
// 	if (i < 0) {
// 		DEBUG(("SplitPathAtEnd: <path> %s does not begin with '/'\n", path));
// 	}
// 	*prefix = debug_malloc(i+1);
// 	*child = debug_malloc(NAME_MAX_LENGTH + 1);
// 	for (j = 0; j < i; j++) {
// 		(*prefix)[j] = path[j];
// 	} (*prefix)[j] = '\0';
// 	if (i == 0) { // Special case, prefix is root
// 		(*prefix)[0] = '/';
// 		(*prefix)[1] = '\0';
// 	}
// 	// Get rid of terminating '/'
// 	if (path[length-1] == '/')
// 		length--;

// 	for (i = 0, j++; j < length && i < NAME_MAX_LENGTH; j++, i++) {
// 		(*child)[i] = path[j];
// 	} (*child)[i] = '\0';
// 	DEBUG(("SplitPathAtEnd: path %s\n\tparent %s\n\tchild %s\n", path, *prefix, *child));
// 	return j;
// }

// /**
//  * Returns the least numbered unused inode. If all inodes in the ifile are in use,
//  * then a new inode is appended to the ifile. 
//  *
//  * If there are no available inodes and the ifile takes up FILE_MAX_SIZE, then
//  * the error code EFBIG is returned.
//  */
// int Directory::NewInode(Inode *inode) {
//     DEBUG(("NewInode\n"));
// 	int	i,
// 		length;
// 	char buf[FILE_MAX_SIZE];
// 	Inode *inodes, 
// 		  *ifile = log_get_ifile_inode();

// 	File_Read(ifile, 0, ifile->fileSize, buf);
// 	inodes = (Inode *) buf;
// 	length = ifile->fileSize / sizeof(Inode);
// 	for (i = 0; i < length; i++) {
// 		if (inodes[i].fileType == NO_FILE) {
// 			DEBUG(("New inode is %d\n", i));
// 			memcpy(inode, inodes + i, sizeof(Inode));
// 			return 0;
// 		}
// 	}
	

// 	// No free inode was found.
// 	if (ifile->fileSize + sizeof(Inode) > FILE_MAX_SIZE)
// 		return -EFBIG; // ifile too big

// 	// Allocate a new inode
// 	memset(inode, 0, sizeof(Inode));
// 	DEBUG(("Made new inode, %d\n", length));
// 	inode->fileType = NO_FILE; // Is 0 anyways, but in case this macro changes	
// 	inode->inum = length;
// 	DEBUG(("NewInode: Writing inode %d to ifile\n", inode->inum));
// 	File_Write(ifile, ifile->fileSize, sizeof(Inode), inode);
// 	DEBUG(("NewInode: Finished writing inode %d\n", inode->inum));
// 	debug_free(ifile);
// 	return 0;
// }

/**
 * Inverts the bit corresponding to inum in inodes_used, and returns
 * the new value.
 */
// int Directory::Directory_ToggleUsedInum(int inum) {
// 	if (inodes_used[inum / 8] == 0) {
// 		inodes_used[inum / 8] = 1;
// 		return 1;
// 	}
// 	else {
// 		inodes_used[inum / 8] = 0;
// 		return 0;
// 	}
// }

