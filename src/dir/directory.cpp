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


Directory::Directory(inputState* state): file(File(*state)) {

}

//This function initializes the directory structure
void* Directory::Initialize(struct fuse_conn_info *conn) {
// void* Directory::Initialize() {


	Inode iFile = file.iFile; // Inode of the Ifile
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
		directoryInode.hardLinkCount = 1;
		file.fileWrite(&directoryInode, 0, 0, NULL);
		file.Flush();

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

int Directory::createHardLink(const char *to, const char *from) {
	int   err, length;

	Inode ino, dir;

	// Before a path at *from is created, check if a directory at that path already exists
	err = file.ReadPath(from, &ino);

	// If it did not return any error, the &ino was assigned with the directory linked by *from. So, this is an error
	if (!err) {
		std::cout << "HardLink: <from> " << from << "already exists\n" << std::endl;
		return -EEXIST;
	}

	length = strlen(from);
	char  path[length+1], name[static_cast<unsigned int>(fileLength::LENGTH)];
	strcpy(path, from);

	int i;
	// Find path to directory of *from
	for (i = length - 1; i > 0 && path[i] != '/'; i--);
	if (i == 0) {
		std::cout << "HardLink: path" << path << "is invalid\n" << std::endl;
		return -ENOTDIR;
	}

	path[i] = '\0';
	i++;

	// Copy name. Name now contains the filename that is to be hardlinked
	int j;
	for (j = 0; path[i+j] != '/' && path[i+j] != '\0'; j++) {
		name[j] = path[i+j];
	} name[j] = '\0';

	// Path  must be valid because it contains 'name'
	err = file.ReadPath(path, &dir);
	if (err) {
		std::cout << "HardLink: Error reading path  " << path << "\n" << std::endl;
		return err;
	}

	// *to should be valid because that is the file whose hardlink we are making
	err = file.ReadPath(to, &ino);
	if (err) {
		std::cout << "HardLink: Error reading <to> " << to << "\n" << std::endl;
		return err;
	}

	err = file.NewEntry(&dir, &ino, name);
	if (err) {
		std::cout << "HardLink: Error" << strerror(err) << "in AddEntry for " << name << "\n" << std::endl;
	}
	ino.hardLinkCount++;

	file.writeInode(&ino);
	return err;
}

/**
 * Creates a symbolic link named <sym> pointing to the file <file>.
 *
 * If <sym> exists, <file> does not exist, or some other error occurs
 * an error code is returned.
 */
int Directory::createSymLink(const char *to, const char *sym) {

	// TODO: Fix. Hypothesis: I should not evaluate the path <file>, just dump it in a new file
	int err;
	Inode ino;
	std::cout << "SymLink: Making link " << sym << "to " << to << "\n" << std::endl;
	err = file.ReadPath(sym, &ino);
	if (!err) {
		std::cout << "SymLink: sym" << sym << "already exists\n" << std::endl;
		return -EEXIST;
	}

 	//S_IFLNK: This is the file type constant of a symbolic link.
	err = file.fileCreate(sym, S_IFLNK, NULL);
	if (err) {
		std::cout << "SymLink: Error creating" << sym << "\n" << std::endl;
		return err;
	}

	err = file.ReadPath(sym, &ino);
	if (err) {
		std::cout << "SymLink: Error reading new created path" << sym << "\n" << std::endl;
		return err;
	}

	int length = strlen(to) + 1;
	std::cout << "SymLink: Writing link\n" << std::endl;
	file.fileWrite(&ino, 0, length, to);
	return 0;
}

int Directory::readLink(const char *path, char *buf, size_t bufsize) {

	if (bufsize < 0) {
		std::cout << "ReadLink: Buffer has negative size " << bufsize << "\n" << std::endl;
		return -EINVAL;
	}

	int err;
	Inode ino;

	err = file.ReadPath(path, &ino);
	if (err) {
		std::cout << "ReadLink: Could not read path " << path << "\n" << std::endl;
		return err;
	}

	if (ino.fileType != static_cast<char>(fileTypes::SYM_LINK)) {
		std::cout << "ReadLink: " << path << " not a sym link\n" << std::endl;
		return -EINVAL;
	}

	char contents[ino.fileSize];
	file.fileRead(&ino, 0, ino.fileSize, contents);

	size_t min = (bufsize < ino.fileSize ? bufsize : ino.fileSize);
	memcpy(buf, contents, min);

	std::cout << "ReadLink: Returning count " << min << "\n" << std::endl;
	return 0;
}

int Directory::splitPathAtEnd(const char *path, char *prefix, char *child) {
	int length = strlen(path);
	int i, j;

	// i starts at 2nd to last character, to skip terminating '/' character if present
	for (i = length - 2; i >= 0 && path[i] != '/'; i--);

	if (i < 0) {
		std::cout << "splitPathAtEnd: path " << path << " does not begin with '/'\n" << std::endl;
	}

	char prefixTmp[i+1], childTmp[static_cast<unsigned int>(fileLength::LENGTH)];
	memcpy(prefix, prefixTmp, i+1);
	memcpy(child, childTmp, static_cast<unsigned int>(fileLength::LENGTH));
	
	for (j = 0; j < i; j++) {
		(prefix)[j] = path[j];
	} 

	(prefix)[j] = '\0';
	
	// If the prefix is the root
	if (i == 0) {
		(prefix)[0] = '/';
		(prefix)[1] = '\0';
	}

	// Get rid of terminating '/'
	if (path[length-1] == '/')
		length--;

	for (i = 0, j++; j < length && i < static_cast<unsigned int>(fileLength::LENGTH); j++, i++) {
		(child)[i] = path[j];
	} 
	
	(child)[i] = '\0';
	
	std::cout << "splitPathAtEnd: path " << path << " parent " << *prefix << " child " << *child << "\n" << std::endl;
	return j;
}

int Directory::removeDirectory(const char *path) {

	Inode dir, parDir;
	char parent[static_cast<unsigned int>(fileLength::LENGTH)], name[static_cast<unsigned int>(fileLength::LENGTH)];

	// Should not be able to remove the root directory
	if (strcmp(path, "/") == 0) {
		std::cout << "removeDirectory: Tried to remove root directory\n" << std::endl;
		return -EACCES;
	}

	int err = file.ReadPath(path, &dir);
	if (err) {
		std::cout << "removeDirectory: Error " << strerror(err) << " reading path " << path << "\n" << std::endl;
		return err;
	}

	if (dir.fileType != static_cast<char>(fileTypes::DIRECTORY)) {
		std::cout << "removeDirectory: " << path << " has filetype " << dir.fileType << "\n" << std::endl;
		return -ENOTDIR;
	}

	if (dir.fileSize > 0) {
		std::cout << "removeDirectory: Directory " << path << " has size " << dir.fileSize << " > 0\n" << std::endl;
		return -ENOTEMPTY;
	}

	splitPathAtEnd(path, parent, name);

	err = file.ReadPath(parent, &parDir);

	if (err) {
		std::cout << "Rmdir: Error " << strerror(err) << " reading parent path " << parent << "\n" << std::endl;
		return err;

	}
	
	deleteEntry(&parDir, &dir, name);

	file.fileDelete(&dir);
	return 0;
}

int Directory::deleteEntry(Inode *dir, Inode *ino, const char *name) {

	std::cout << "deleteEntry: " << name << "\n" << std::endl;

	off_t offset;
	int inum, nRead;
	char dbuf[dir->fileSize], tname[static_cast<unsigned int>(fileLength::LENGTH) + 1];

	file.fileRead(dir, 0, dir->fileSize, dbuf);
	
	// Debugging code
	// nRead = 0;
	// while (nRead < dir->fileSize) {
	// 	printf("inum = %d. Name = %s\n", (int) dbuf[nRead], dbuf + nRead+sizeof(int));
	// 	nRead += sizeof(int) + strlen(dbuf+nRead+sizeof(int)) + 1;
	// }

	tname[0] = '\0';
	offset = 0;

	// while tname is before name
	while (strcmp(tname, name) < 0 && offset < dir->fileSize) {
		innerReadDir(dbuf + offset, &inum, tname, &nRead);
		offset += nRead;

		std::cout << "deleteEntry: Read name " << tname << " Inum "<< inum << std::endl;
		std::cout << "Offeset = " << offset << ", fileSize =  " << dir->fileSize << "\n" << std::endl;
	}

	if (strcmp(tname, name) != 0) {
		std::cout << "deleteEntry: File " << name << " not found in parent.\n" << std::endl;
		return -ENOENT;
	}

	// Write modified directory
	char newBuf[dir->fileSize - offset];
	int length = dir->fileSize;
	memcpy(newBuf, dbuf + offset, dir->fileSize - offset);
	file.fileWrite(dir, offset - nRead, dir->fileSize - offset, newBuf);
	file.Truncate(dir, length - nRead);

	// Decrement link count
	ino->hardLinkCount--;

	return 0;
}

/**
 * Removes the directory entry specified by path. Currently, this deletes the file. In phase 2, 
 * the file will only be deleted if <path> was the last hard link to it.
 */
int Directory::unlink(const char *path) {
	Inode inode, dir;

	char name[static_cast<unsigned int>(fileLength::LENGTH)], prefix[static_cast<unsigned int>(fileLength::LENGTH)];

	splitPathAtEnd(path, prefix, name);
  
	int err = file.ReadPath(path, &inode);

	std::cout << "Unlink: Path " << path << "\n\tParent " << prefix << "\n\tName " << name << "\n" << std::endl;

	if (err) {
		std::cout << "Unlink: Error " << strerror(err) << " reading path " << path << "\n" << std::endl;	
		return err;
	}

	err = file.ReadPath(prefix, &dir);
	if (err) {
		std::cout << "Unlink: Error " << strerror(err) << " reading path " << prefix << "\n" << std::endl;
		return err;
	}

  // remove inode from directory
  err = deleteEntry(&dir, &inode, name);
	if (err) {
		std::cout << "Unlink: Error " << strerror(err) << " in deleteEntry " << path << "\n" << std::endl;
		return err;
	}

  // Free inode if no links. TODO: Change for phase 2.
	if (inode.hardLinkCount < 1) {
		err = file.fileDelete(&inode);
	}

  return err;
}

int Directory::rename(const char *from, const char *to) {
	char fDirPath[static_cast<unsigned int>(fileLength::LENGTH)], fName[static_cast<unsigned int>(fileLength::LENGTH)], tDirPath[static_cast<unsigned int>(fileLength::LENGTH)], tName[static_cast<unsigned int>(fileLength::LENGTH)];

	Inode fDir, tDir, ino, tino;

	int err;

	splitPathAtEnd(from, fDirPath, fName);
	splitPathAtEnd(to, tDirPath, tName);

	std::cout << "Directory Rename:\n\tFrom: " << from << "\n\tParent: " << fDirPath << "\n\tName: " << fName << "\n\tTo: " << to << "\n\tParent: " << tDirPath << "\n\tName: " << tName << "\n" << std::endl;

	err = file.ReadPath(fDirPath, &fDir);
	if (err) {
		std::cout << "Rename: Error " << strerror(err) << " while reading path " << fDirPath << "\n" << std::endl;
		return  err;
	}

	err = file.ReadPath(from, &ino);
	if (err) {
		std::cout << "Rename: Error " << strerror(err) << " while reading path " << from << "\n" << std::endl;
		return err;
	}

	// TODO: Support all file types
	if (ino.fileType != static_cast<char>(fileTypes::PLAIN_FILE)) {
		std::cout << "Rename: " << from << " has invalid filetype " << ino.fileType << "\n" << std::endl;
		return -EINVAL;
	}

	err = file.ReadPath(tDirPath, &tDir);
	if (err) {
		std::cout << "Rename: Error " << strerror(err) << " while reading path " << tDirPath << "\n" << std::endl;
		return err;
	}

	err = file.ReadPath(to, &tino); // Just check if <to> already exists
	if (err == -ENOENT || err == ENOENT) {
		std::cout << "Rename: File " << to << "does not exist.\n" << std::endl;
	}
	else if (err) {
		std::cout << "Rename: Error " << strerror(err) << " while reading path " << to << "\n" << std::endl;
		return err;
	} else if (tino.inum != ino.inum) { // <to> is a file, delete it unless is a hardlink to <from>
		if (tino.fileType != static_cast<char>(fileTypes::PLAIN_FILE)) {
			std::cout << "Rename: " << to << " has invalid fileType " << tino.fileType << "\n" << std::endl;
			return -EINVAL;
		}
		file.fileDelete(&tino);
	}

	// Now rename ino
	// TODO: Inefficient!! Unacceptable!!
	deleteEntry(&fDir, &ino, fName);

	std::cout << "Rename: DeleteEntry finished.\n" << std::endl;
	if (fDir.inum == tDir.inum) { // not moving directory. fDir was updated by DeleteEntry but tDir was not.
		file.NewEntry(&fDir, &ino, tName);
	} else {
		file.NewEntry(&tDir, &ino, tName);
	}
	return 0;
}