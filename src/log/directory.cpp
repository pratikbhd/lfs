#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>

#include "directory.h"

#define INODES_PER_SEGMENT SEGMENT_SIZE/sizeof(Inode)

// Local declarations
int inodes_length, // Total number of inodes allocated, excluding the ifile
    *inodes_used; // A bit vector describing which inodes are used.

Inode *alloc_inode();
int internalReadDir(char *, int *, char *, int *);
int inodeByNameInDirBuf(const char *buf, int length, const char *name, int *inum);


/*
 * This function initializes the directory structure
 */
// TODO: Highly dependant on log layer. Will have to wait for Rahul's implementation of log layer.
void *Directory::Directory_Initialize(struct fuse_conn_info *conn) {
	DEBUG(("Initialize Directory\n"));
	log_init_layer_default();
    
    DEBUG(("INITDIR LOG INFO\n"));
    log_print_info(lInfo);
    
	Inode *ifile = log_get_ifile_inode(),
		  *dirIno,
	      *inodes;

	inodes_length = BLOCK_SIZE / sizeof(Inode);
	inodes_used = debug_calloc(INODES_MAX_NUM / sizeof(char) + (INODES_MAX_NUM % sizeof(char) == 0 ? 0 : 1),
			sizeof(char)); // bit vector, tracks usage of all inodes

	if (ifile->fileSize == 0) { // Create dirIno
		inodes_length = 1;
		Directory_ToggleInumIsUsed(ROOT_INODE);
		dirIno = debug_calloc(1, sizeof(Inode));
		dirIno->inum = ROOT_INODE;
		dirIno->fileType = DIRECTORY;
		dirIno->fileSize = 0;
		dirIno->hardLinkCount = 1;
		File_Write(dirIno, 0, 0, NULL);
		debug_free(dirIno);
		dirIno = Directory_GetInode(ROOT_INODE);
		DEBUG(("initDir: root file type after write is %d\n", dirIno->fileType));
		debug_free(ifile);
		debug_free(dirIno);
	} else {
		inodes = debug_malloc(ifile->fileSize * sizeof(char));
		File_Read(ifile, 0, ifile->fileSize, inodes);
		inodes_length = 0;
		int i;
		for (i = 0; i < ifile->fileSize / sizeof(Inode); i++) {
			if (inodes[i].fileType != NO_FILE) {
				Directory_ToggleInumIsUsed(i);
				inodes_length++;
			}
		}
		debug_free(inodes);
		debug_free(ifile);
	}
	return 0;
}


int Directory::Directory_MakeDir(const char *path, mode_t mode) {
	Inode dir;
	int err = Directory::Directory_ReadPath(path, &dir);

	if (!err) {
		DEBUG(("MakeDir: File %s already exists\n", path));
		return -EEXIST;
	}
	err = File_Create(path, S_IFDIR, NULL);
	if (err) {
		DEBUG(("MakeDir: Could not create dir %s\n", path));
		return err;
	}
	err = Directory_ReadPath(path, &dir);
	if (err) {
		DEBUG(("MakeDir: Directory %s was not created\n", path));
	} else {
		DEBUG(("MakeDir:\n"));
		DEBUG(("\tinum = %d\n", dir.inum));
		DEBUG(("\ttype = %s\n", (dir.fileType == DIRECTORY ? "Directory" : "File")));
	}
	return 0;
}


/**
 * Reads <length> bytes from the file specified by <path> starting at <offset> into <buffer>.
 */
int Directory::Directory_Read(const char *path, char *buffer, size_t length, off_t offset,
                struct fuse_file_info *fi) {
    DEBUG(("DirectoryRead: length = %lu, offset = %lu\n", length, offset));
    Inode inode;
    int err = Directory_ReadPath(path, &inode);
        
    if (err) {
            return err;
    }

    int val = File_Read(&inode, offset, length, buffer);

    return val;
}


/**
 * Writes <length> bytes from <buffer> to the file specified by <path> starting at <offset>.
 */
int Directory::Directory_Write(const char *path, const char *buffer, size_t length, off_t offset,
                struct fuse_file_info *fi) {
        DEBUG(("DirectoryWrite\n"));
        Inode inode;
        int err = Directory_ReadPath(path, &inode);
        if (err) {
            return err;
        }

        int val = File_Write(&inode, offset, length, buffer);

        return val;
}


/**
 * Stores an inode for the file specified by path in the inode parameter, or NULL if no such file exists.
 * In that case, -ENOENT is returned, otherwise 0. Space is allocated for <inode> and must be freed by
 * the caller.
 */
int Directory::Directory_ReadPath(const char *path, Inode *inode) {
	char directory[FILE_MAX_SIZE], // TODO: Confirm this
		 name[NAME_MAX_LENGTH+1]; // TODO: Confirm this as well
	int inum
		, length
		, offset
		, i
		, err
		, pathLen = strlen(path);
	
	// Check if path always starts from '/'		
    if (path[0] != '/') {
		DEBUG(("ReadPath: Invalid path, begins with %c\n", path[0]));
		return -EINVAL;
	}
	
	Inode *dirIno = Directory_GetInode(ROOT_INODE);
	
	// Mounting to the root directory i.e. '\'
	if (path[1] == '\0') { // Just reading the dirIno
		DEBUG(("Read path on directory %s\n", path));
		DEBUG(("Root->inum = %d, length = %lu\n", dirIno->inum, dirIno->fileSize));
		memcpy(inode, dirIno, sizeof(Inode));
		return 0;
	}

	// Read directory for next part of path. Initially directory is the root
	offset = 1;
	length = File_Read(dirIno, 0, dirIno->fileSize, directory);
	DEBUG(("ReadPath: dir is length %d\n", length));
	DEBUG(("\tContents: "));
	char tmp = directory[length];
	directory[length] = '\0';
	DEBUG(("%s\n", directory));
	directory[length] = tmp;

	// I don't know what's happening here
	while (offset < pathLen) {
		for (i = 0; (path + offset)[i] != '/' && (path + offset)[i] != '\0'; i++) { // Copy a name from the path
			name[i] = (path + offset)[i];
		} name[i] = '\0'; // If (path + offset)[i] or next char is null, then end of path. This is inode to return
		DEBUG(("ReadPath: getting inode for %s\n", name));
		err = inodeByNameInDirBuf(directory, length, name, &inum);
		debug_free(dirIno);
		if (err) {
			DEBUG(("ReadPath: Could not follow path %s\nFailed to read %s\n", path, name));
			return err;
		}
		dirIno = Directory_GetInode(inum);
		if ((path + offset)[i] == '\0' || (path + offset)[i+1] == '\0') { // Found it, last char may be '/'
			DEBUG(("ReadPath: Returning inode %d\n", dirIno->inum));
			memcpy(inode, dirIno, sizeof(Inode));
			debug_free(dirIno);
			return 0;
		}
		else if (dirIno->fileType != DIRECTORY) {
			DEBUG(("ReadPath: Link %s in path %s not a directory\n", name, path));
			debug_free(dirIno);
			return -ENOTDIR;
		}
		offset += i + 1; // Advance to next dir in path
		length = File_Read(dirIno, 0, dirIno->fileSize, directory);
	}
	// Didn't find it
	debug_free(dirIno);
	return -ENOENT;
}


/**
 * The readdir operation in the fuse_operations type. offset and finfo are ignored,
 * and directories are always read in their entirety.
 */
int Directory::Directory_Readdir(const char *path, 
					  void *buf, 
					  fuse_fill_dir_t filler, 
       				  off_t offset, 
					  struct fuse_file_info *finfo) {

    // TODO: Fix for phase 2
	DEBUG(("Readdir %s\n", path));
	DEBUG(("##################\n"));
	DEBUG(("bytesPerBlock = %d\n", lInfo->bytesPerBlock));
	DEBUG(("File max size = %lu\n", FILE_MAX_SIZE));
	DEBUG(("##################\n"));

	int   i
		, length
		, count
		, nRead
		, inum;

	struct stat st;

    char buffer[FILE_MAX_SIZE], 
         name[256];

	Inode dir;
	int err = Directory_ReadPath(path, &dir);
	if (err) {
		DEBUG(("Readdir: Could not find path %s\n", path));
		return err;
	} else if (dir.fileType != DIRECTORY) {
		DEBUG(("Readdir: %s is not a directory. Filetype is %d\n", path, dir.fileType));
		return -ENOTDIR;
	}
	/*
	File_Read(mntpnt, 0, mntpnt->fileSize, buffer);
	length = mntpnt->fileSize;
	memset(&st, 0, sizeof(st));
	st.st_ino = ROOT_INODE;
	inum = ROOT_INODE;
*/
	printInode(&dir);
	st.st_ino = dir.inum;
	File_Read(&dir, 0, dir.fileSize, buffer);
    filler(buf, ".", &st, 0);
    filler(buf, "..", &st, 0);
	DEBUG(("Readdir: Filled buf with . ..\n"));
	count = 0;
	length = dir.fileSize;
	DEBUG(("Readdir: Dir %s has size %lu\n", path, dir.fileSize));
    
	// parse dirents read from directory
    for (i = 0; i < length; i += nRead) {
		internalReadDir(buffer + i, &inum, name, &nRead); 
		st.st_ino = inum;
		filler(buf, name, &st, 0);
		count++;
    }
	DEBUG(("Readdir: Return, count %d\n", count));
    return 0; //count;
}

/**
 * Inserts the pair <file->inum,fileName> into the directory associated with <dir>.
 */
int Directory::Directory_AddEntry(Inode *dir, Inode *file, const char *fileName) {
	DEBUG(("AddEntry %s\n", fileName));
#ifdef LFSDEBUG
	printf("Directory Inode:\n");
	printInode(dir);
	printf("File Inode:\n");
	printInode(file);
#endif
	int length, i, inum, insertAt, cmp;
   
	length = strlen(fileName);
    fprintf(stderr, "length: %d\n", length);
    fprintf(stderr, "dirfilesize: %lu\n", dir->fileSize);
    
    /* dir->fileSize gets really big when add 300 files, delete them all, then create 300 more */
	char buf[dir->fileSize];
    char name[NAME_MAX_LENGTH+1];
    char *newBuf;

	DEBUG(("AddEntry: length = %d\n", length));
	File_Read(dir, 0, dir->fileSize, buf);  	

	DEBUG(("AddEntry: Inserting, dir size = %lu\n", dir->fileSize));
	insertAt = 0;
	while (insertAt < dir->fileSize) {
		memcpy(&inum, buf + insertAt, sizeof(int));
		strcpy(name, buf + insertAt + sizeof(int));
		//insertAt += strlen(name) + 1; // Leave room for null
		//insertAt += sizeof(int);
		

		cmp = strcmp(fileName, name);
		if (cmp < 0) {
			break; // Found place to insert next entry
		} else if (cmp == 0) {
			DEBUG(("AddEntry: Adding existant file %s\n", fileName));
			return -EEXIST;
		}
		insertAt += sizeof(int) + strlen(name) + 1; // skip inum, name and null byte.
	}
	// Going to write everything in the file starting from the new entry
	newBuf = debug_malloc(sizeof(int) + length + 1 + dir->fileSize - insertAt); // Room for inum, fileName + null byte, and rest of file
	// Copy next entry into buf
	DEBUG(("AddEntry: Adding inum %d, name %s\n", file->inum, fileName));
	memcpy(newBuf, &(file->inum), sizeof(int));
	DEBUG(("\t1st memcpy done\n"));
	memcpy(newBuf + sizeof(int), fileName, length+1);
	DEBUG(("\t2nd memcpy done\n"));
// Copy rest of directory 
	memcpy(newBuf + sizeof(int) + length + 1
		, buf+insertAt
		, dir->fileSize - insertAt);

	DEBUG(("\t3rd memcpy done\n"));

	// Changed this from file to dir
	File_Write(dir, insertAt, dir->fileSize - insertAt + sizeof(int) + length + 1, newBuf); 
	debug_free(newBuf);
	DEBUG(("Added entry %s\nContents of buffer: inum = %d and name = %s\n", 
				fileName, 
				*((int*) newBuf), 
				(newBuf + sizeof(int))));
#ifdef LFSDEBUG
	printf("AddEntry: Contents after insertion of %s:\n", fileName);
	for (i = 0; i < dir->fileSize - insertAt + sizeof(int) + length + 1; i++) {
		putchar(newBuf[i]);
	} putchar('\n');
#endif
	return 0;	
}

/**
 * Finds the first item found in the directory. inum will point to the inode number, name to the
 * file name, and newOffset to the point in directory where the next entry begins. name should be
 * buffer of size at least NAME_MAX_LENGTH+1.
 */
int Directory::internalReadDir(char *directory, int *inum, char *name, int *lengthRead) {
	DEBUG(("internalReadDir\n"));
	char *startName = directory + sizeof(int);
	int i;
	
	memcpy(inum, directory, sizeof(int));
	*lengthRead = sizeof(int);
	DEBUG(("lengthRead starts %d\n", *lengthRead));
	
	for (i = 0; startName[i] != '\0'; i++, (*lengthRead)++) {
		DEBUG(("Iterate readDir, read char %c\n", startName[i]));
		name[i] = startName[i];
	}
	name[i] = '\0';
	(*lengthRead)++;
	DEBUG(("internalReadDir: Return, lengthRead = %d\n", *lengthRead));
	return 0;
}

// TODO: This
int Directory::Directory_Opendir(const char *name, struct fuse_file_info *finfo) {
	DEBUG(("Opendir %s\n", name));
	int err = File_Open(name, finfo);
	if (err) {
		DEBUG(("Opendir: Could not open %s\n", name));
		return err;
	}
	Inode dir;
	err = Directory_ReadPath(name, &dir);
	if (err) {
		DEBUG(("Opendir: Could not read path %s\n", name));
		return err;
	}

	if (dir.fileType != DIRECTORY) {
		DEBUG(("Opendir: %s is not a directory. Filetype = %d\n", name, dir.fileType));
		return -ENOTDIR;
	}
    return 0;
}

/**
 * Given a buffer <buf> of <length> bytes populated by the internalReadDir function, stores the inum of the file 
 * with the given <name> at the location pointed to by <inum>.
 *
 * Returns 0 if <inum> is found, otherwise -ENOENT.
 */
int Directory::inodeByNameInDirBuf(const char *buf, int length, const char *name, int *inum) {
	int num
		, offset;
	char tmpName[NAME_MAX_LENGTH+1];
	
	if (name[0] == '\0') {
		DEBUG(("Null name in 'inodeByNameInDirBuf'\n"));
		return -ENOENT; // No name
	}

	tmpName[0] = '\0';
	// Search for name
	offset = 0;
	while (strcmp(name,tmpName) != 0 && offset < length) {
		memcpy(&num, buf + offset, sizeof(int));
		offset += sizeof(int);
		strcpy(tmpName, buf + offset);
		offset += strlen(tmpName) + 1;
	}

	if (strcmp(name,tmpName) == 0) {
		DEBUG(("inodeByName: inum = %d\n", num));
		*inum = num;
		return 0;
	}
	// Not found
	return -ENOENT;
}


int Directory::Directory_HardLink(const char *to, const char *from) {
	int   err
		, length;

	Inode ino
		, dir;

	err = Directory_ReadPath(from, &ino);
	if (!err) {
		DEBUG(("HardLink: <from> %s already exists\n", from));
		return -EEXIST;
	}
	length = strlen(from);
	char  path[length+1]
		, name[NAME_MAX_LENGTH];
	strcpy(path, from);
	int i;
	// Find path to directory of from
	for (i = length - 1; i > 0 && path[i] != '/'; i--);
	if (i == 0) {
		DEBUG(("HardLink: path %s invalid\n", path));
		return -ENOTDIR;
	}
	path[i] = '\0';
	i++;
	// Copy name
	int j;
	for (j = 0; path[i+j] != '/' && path[i+j] != '\0'; j++) {
		name[j] = path[i+j];
	} name[j] = '\0';

	err = Directory_ReadPath(path, &dir);
	if (err) {
		DEBUG(("HardLink: Error reading path %s\n", path));
		return err;
	}
	err = Directory_ReadPath(to, &ino);
	if (err) {
		DEBUG(("HardLink: Error reading <to> %s\n", to));
		return err;
	}
	err = Directory_AddEntry(&dir, &ino, name);
	if (err) DEBUG(("HardLink: Error %s in AddEntry for %s\n", strerror(err), name));
	ino.hardLinkCount++;
	writeInode(&ino);
	return err;
}

/**
 * Creates a symbolic link named <sym> pointing file the file <file>.
 *
 * If <sym> exists, <file> does not exist, or some other error occurs
 * an error code is returned.
 */
int Directory::Directory_SymLink(const char *file, const char *sym) {
	// TODO: Fix. Hypothesis: I should not evaluate the path <file>, just dump it in a new file
	int err;
	Inode ino;
	DEBUG(("SymLink: Making link %s to %s\n", sym, file));
   	err = Directory_ReadPath(sym, &ino);
	if (!err) {
		DEBUG(("SymLink: sym %s already exists\n", sym));
		return -EEXIST;
	}
/*	err = Directory_ReadPath(file, &ino);
	if (err) {
		DEBUG(("SymLink: file %s does not exist\n", file));
		return err;
	}*/
	err = File_Create(sym, S_IFLNK, NULL);
	if (err) {
		DEBUG(("SymLink: Error creating %s\n", sym));
		return err;
	}
	err = Directory_ReadPath(sym, &ino);
	if (err) {
		DEBUG(("SymLink: Error reading new created path %s\n", sym));
		return err;
	}
	int length = strlen(file) + 1;
	DEBUG(("SymLink: Writing link\n"));
	File_Write(&ino, 0, length, file);   
	return 0;	
}

int Directory::Directory_ReadLink(const char *path, char *buf, size_t bufsize) {
	if (bufsize < 0) {
		DEBUG(("ReadLink: Buffer has negative size %lu\n", bufsize));
		return -EINVAL;
	}
	int err;
	Inode ino;
	err = Directory_ReadPath(path, &ino);
	if (err) {
		DEBUG(("ReadLink: Could not read path %s\n", path));
		return err;
	}
	if (ino.fileType != SYM_LINK) {
		DEBUG(("ReadLink: %s not a sym link\n", path));
		printInode(&ino);
		return -EINVAL;
	}
	char contents[ino.fileSize];
	File_Read(&ino, 0, ino.fileSize, contents);

//	int i;
	size_t min = (bufsize < ino.fileSize ? bufsize : ino.fileSize);
	memcpy(buf, contents, min);
//	for (i = 0; i < bufsize && i < ino.fileSize; i++) {
//		buf[i] = contents[i];
//	}

	DEBUG(("ReadLink: Returning count %lu\n", min));
	return 0;//min;

}

Inode *Directory::Directory_GetInode(int inum) {
	Inode *inode = (Inode*) debug_malloc(sizeof(Inode)),
		  *ifile;
	ifile = log_get_ifile_inode();
	int length = File_Read(ifile, inum*sizeof(Inode), sizeof(Inode), inode);  
	DEBUG(("GetInode: Read %d bytes from ifile, inum = %d and inode->inum = %d\n", length, inum, inode->inum));
    printInode(inode);
	debug_free(ifile);
	return inode;
}



void Directory::Directory_Destroy(void *data) {
	DEBUG(("Destroy\n"));
	debug_free(inodes_used);
    log_close_layer();
}
//    DEBUG(("Directory_Destroy() not implemented\n"));
//	debug_free(inodes_used);
//    exit(0);


/**
 * Returns 1 if the inode with number inum is used, and 0 otherwise.
 */
int Directory::Directory_InumIsUsed(int inum) {
    return inodes_used[inum / 8] & (1 << (inum % 8));
}

int Directory::Directory_DeleteEntry(Inode *dir, Inode *ino, const char *name) {
	DEBUG(("DeleteEntry: %s\n", name));
	off_t offset;

	int   inum
		, nRead;

	char dbuf[dir->fileSize]
		, tname[NAME_MAX_LENGTH + 1];
	File_Read(dir, 0, dir->fileSize, dbuf);
	
#ifdef LFSDEBUG
	nRead = 0;
	while (nRead < dir->fileSize) {
		printf("inum = %d. Name = %s\n", (int) dbuf[nRead], dbuf + nRead+sizeof(int));
		nRead += sizeof(int) + strlen(dbuf+nRead+sizeof(int)) + 1;
	}
#endif

	tname[0] = '\0';
	offset = 0;
	// while tname is before name
	while (strcmp(tname, name) < 0 && offset < dir->fileSize) {
		internalReadDir(dbuf + offset, &inum, tname, &nRead);
		offset += nRead;
		DEBUG(("DeleteEntry: Read name %s. Inum %d\n", tname, inum));
		DEBUG(("\tOffset = %lu, filesize = %lu\n", offset, dir->fileSize));
	}
	if (strcmp(tname, name) != 0) {
		DEBUG(("DeleteEntry: File %s not found in parent\n", name));
		return -ENOENT;
	}
	// Write modified directory
	char newBuf[dir->fileSize - offset];
	int length = dir->fileSize;
	memcpy(newBuf, dbuf + offset, dir->fileSize - offset);
	File_Write(dir, offset - nRead, dir->fileSize - offset, newBuf);
	File_Truncate(dir, length - nRead);

	// Decrement link count
	ino->hardLinkCount--;

	return 0;
}

int Directory::Directory_MakeDir(const char *path, mode_t mode) {
	DEBUG(("MakeDir\n"));
	Inode dir;
	int err = Directory_ReadPath(path, &dir);	

	if (!err) {
		DEBUG(("MakeDir: File %s already exists\n", path));
		return -EEXIST;
	}
	err = File_Create(path, S_IFDIR, NULL);
	if (err) {
		DEBUG(("MakeDir: Could not create dir %s\n", path));
		return err;
	}
	err = Directory_ReadPath(path, &dir);
	if (err) {
		DEBUG(("MakeDir: Directory %s was not created\n", path));
	} else {
		DEBUG(("MakeDir:\n"));
		DEBUG(("\tinum = %d\n", dir.inum));
		DEBUG(("\ttype = %s\n", (dir.fileType == DIRECTORY ? "Directory" : "File")));
	}
	return 0;
}

/**
 * Inverts the bit corresponding to inum in inodes_used, and returns
 * the new value.
 */
int Directory::Directory_ToggleInumIsUsed(int inum) {
	inodes_used[inum / 8] ^= (1 << (inum % 8));
	return Directory_InumIsUsed(inum);
}

/**
 * Returns the least numbered unused inode. If all inodes in the ifile are in use,
 * then a new inode is appended to the ifile. 
 *
 * If there are no available inodes and the ifile takes up FILE_MAX_SIZE, then
 * the error code EFBIG is returned.
 */
int Directory::Directory_NewInode(Inode *inode) {
    DEBUG(("NewInode\n"));
	int	i,
		length;
	char buf[FILE_MAX_SIZE];
	Inode *inodes, 
		  *ifile = log_get_ifile_inode();

	File_Read(ifile, 0, ifile->fileSize, buf);
	inodes = (Inode *) buf;
	length = ifile->fileSize / sizeof(Inode);
	for (i = 0; i < length; i++) {
		if (inodes[i].fileType == NO_FILE) {
			DEBUG(("New inode is %d\n", i));
			memcpy(inode, inodes + i, sizeof(Inode));
			return 0;
		}
	}
	

	// No free inode was found.
	if (ifile->fileSize + sizeof(Inode) > FILE_MAX_SIZE)
		return -EFBIG; // ifile too big

	// Allocate a new inode
	memset(inode, 0, sizeof(Inode));
	DEBUG(("Made new inode, %d\n", length));
	inode->fileType = NO_FILE; // Is 0 anyways, but in case this macro changes	
	inode->inum = length;
	DEBUG(("NewInode: Writing inode %d to ifile\n", inode->inum));
	File_Write(ifile, ifile->fileSize, sizeof(Inode), inode);
	DEBUG(("NewInode: Finished writing inode %d\n", inode->inum));
	debug_free(ifile);
	return 0;
}

/**
 * Sets every member of the associated inode to 0 except for its inum.
 */
int Directory::Directory_ResetInode(Inode *inode) {
    DEBUG(("ResetInode\n"));
	int i;
	inode->fileSize = 0;

	for (i = 0; i < 4; i++) {
		inode->block_pointers[i].segmentNumber = 0;
		inode->block_pointers[i].blockOffset = 0;
//		inode->name[i] = '\0';
	}
    inode->indirect_block.segmentNumber = 0;
    inode->indirect_block.blockOffset = 0;
//	for ( ; i < 8; i++) {
//		inode->name[0] = '\0';	
//	}

	inode->hardLinkCount = 0;
	//inode->indirect_block = 0;
	inode->fileType = NO_FILE;
	writeInode(inode);

	return 0;
}

int Directory::writeInode(Inode *inode) {
	DEBUG(("writeInode, size = %lu\n", inode->fileSize));
	Inode *ifile = log_get_ifile_inode();
	File_Write(ifile, inode->inum * sizeof(Inode), sizeof(Inode), inode);
	debug_free(ifile);
	return 0;
}

/**
 * 
 */
int Directory::Directory_Truncate(const char *path, off_t size) {
	DEBUG(("FileTruncate\n"));
	Inode inode;
	int err = Directory_ReadPath(path, &inode);
	if (err) {
		return err;
	}
	err = File_Truncate(&inode, size);
	return err;
}
/**
 * Removes the directory entry specified by path. Currently, this deletes the file. In phase 2, 
 * the file will only be deleted if <path> was the last hard link to it.
 */
int Directory::Directory_Unlink(const char *path) {
    Inode inode
		, dir;

	char *name
		, *prefix;

	Directory_SplitPathAtEnd(path, &prefix, &name);
    int err = Directory_ReadPath(path, &inode);

	DEBUG(("Unlink: Path %s\n\tParent %s\n\tName %s\n", path, prefix, name));
	if (err) {	
		DEBUG(("Unlink: Error %s reading path %s\n", strerror(err), path));
		return err;
	}
	err = Directory_ReadPath(prefix, &dir);
	if (err) {
		DEBUG(("Unlink: Error %s reading path %s\n", strerror(err), prefix));
		return err;
	}

    // remove inode from directory
    err = Directory_DeleteEntry(&dir, &inode, name);
	if (err) {
		DEBUG(("Unlink: Error %s in DeleteEntry %s\n", strerror(err), path));
		return err;
	}

    // Free inode if no links. TODO: Change for phase 2.
	if (inode.hardLinkCount < 1) {
		err = File_Delete(&inode);
	}
	debug_free(name);
	debug_free(prefix);
    return err;

}

/*
 * Counts all inodes in use, except for the ifile inode.
 */
int Directory::Directory_CountInodes() {
	int i, j, count = 0;

	for (i = 0; i < inodes_length; i++) {
		for (j = 0; j < 8; j++) {
			count += (inodes_used[i] >> j) & 1;
		}
	}
	return count;
}

int Directory::Directory_SplitPathAtEnd(const char *path, char **prefix, char **child) {
	int length = strlen(path);
	int i, j;

	// i starts at 2nd to last character, to skip terminating '/' character if present
	for (i = length - 2; i >= 0 && path[i] != '/'; i--);
	if (i < 0) {
		DEBUG(("SplitPathAtEnd: <path> %s does not begin with '/'\n", path));
	}
	*prefix = debug_malloc(i+1);
	*child = debug_malloc(NAME_MAX_LENGTH + 1);
	for (j = 0; j < i; j++) {
		(*prefix)[j] = path[j];
	} (*prefix)[j] = '\0';
	if (i == 0) { // Special case, prefix is root
		(*prefix)[0] = '/';
		(*prefix)[1] = '\0';
	}
	// Get rid of terminating '/'
	if (path[length-1] == '/')
		length--;

	for (i = 0, j++; j < length && i < NAME_MAX_LENGTH; j++, i++) {
		(*child)[i] = path[j];
	} (*child)[i] = '\0';
	DEBUG(("SplitPathAtEnd: path %s\n\tparent %s\n\tchild %s\n", path, *prefix, *child));
	return j;
}

int Directory::Directory_Rmdir(const char *path) {
	Inode dir
		, parDir;
	char *parent
		, *name;
	if (strcmp(path, "/") == 0) {
		DEBUG(("Rmdir: Tried to remove root directory\n"));
		return -EACCES;
	}
	int err = Directory_ReadPath(path, &dir);

	if (err) {
		DEBUG(("Rmdir: Error %s reading path %s\n", strerror(err), path));
		return err;
	}
	if (dir.fileType != DIRECTORY) {
		DEBUG(("Rmdir: %s has filetype %d\n", path, dir.fileType));
		return -ENOTDIR;
	}
	if (dir.fileSize > 0) {
		DEBUG(("Rmdir: Dir %s has size %lu > 0\n", path, dir.fileSize));
		return -ENOTEMPTY;
	}
	Directory_SplitPathAtEnd(path, &parent, &name);
	err = Directory_ReadPath(parent, &parDir);
	if (err) {
		DEBUG(("Rmdir: Error %s reading parent path %s\n", strerror(err), parent));
		return err;
	}
	Directory_DeleteEntry(&parDir, &dir, name);

	File_Delete(&dir);
	debug_free(parent);
	debug_free(name);
	return 0;
}

int Directory::Directory_Rename(const char *from, const char *to) {
	char *fDirPath
		, *fName
		, *tDirPath
		, *tName;

	Inode fDir
		, tDir
		, ino
		, tino;

	int err,
		returnval = 0;

	Directory_SplitPathAtEnd(from, &fDirPath, &fName);
	Directory_SplitPathAtEnd(to, &tDirPath, &tName);
	DEBUG(("DirectoryRename:\n\tfrom: %s\n\tparent:%s\n\tname:%s\n\tto: %s\n\tparent: %s\n\tname:%s\n", 
				from, fDirPath, fName,
				to, tDirPath, tName));

	err = Directory_ReadPath(fDirPath, &fDir);
	if (err) {
		DEBUG(("Rename: Error %s while reading path %s\n", strerror(err), fDirPath));
		returnval = err;
		goto done;
	}
	err = Directory_ReadPath(from, &ino);
	if (err) {
		DEBUG(("Rename: Error %s while reading path %s\n", strerror(err), from));
		returnval = err;
		goto done;
	}
	// TODO: Support all file types
	if (ino.fileType != PLAIN_FILE) {
		DEBUG(("Rename: %s has invalid fileType %d\n", from, ino.fileType));
		returnval = -EINVAL;
		goto done;
	}
	err = Directory_ReadPath(tDirPath, &tDir);
	if (err) {
		DEBUG(("Rename: Error %s while reading path %s\n", strerror(err), tDirPath));
		returnval = err;
		goto done;
	}
	err = Directory_ReadPath(to, &tino); // Just check if <to> already exists
	if (err == -ENOENT || err == ENOENT) {
		DEBUG(("Rename: File %s doesn't exist\n", to));
	}
	else if (err) {
		DEBUG(("Rename: Error %s while reading path %s\n", strerror(err), to));
		returnval = err;
		goto done;
	} else if (tino.inum != ino.inum) { // <to> is a file, delete it unless is a hardlink to <from>
		if (tino.fileType != PLAIN_FILE) {
			DEBUG(("Rename: %s has invalid fileType %d\n", to, tino.fileType));
			returnval = -EINVAL;
			goto done;
		}
		File_Delete(&tino);
	}

	// Now rename ino
	// TODO: Inefficient!! Unacceptable!!
	Directory_DeleteEntry(&fDir, &ino, fName);
	DEBUG(("Rename: DeleteEntry finished\n"));
	if (fDir.inum == tDir.inum) { // not moving directory. fDir was updated by DeleteEntry but tDir was not.
		Directory_AddEntry(&fDir, &ino, tName);
	} else {
		Directory_AddEntry(&tDir, &ino, tName);
	}
	returnval = 0;
done:
	debug_free(fDirPath);
	debug_free(fName);
	debug_free(tDirPath);
	debug_free(tName);
	return returnval;
}


/**
 * Prints all the data associated with the inode.
 */
void Directory::printInode(Inode *inode) {
	DEBUG(("-----------Print Inode------------\n"));
	DEBUG(("- fileType = %d\n", inode->fileType));
	DEBUG(("- inum = %d\n", inode->inum));
	DEBUG(("- hardLinkCount %d\n", inode->hardLinkCount));
	DEBUG(("- size = %lu\n", inode->fileSize));
	//DEBUG(("- blockPointers = [%d, %d, %d, %d]\n", inode->block_pointers[0],inode->block_pointers[1],
	//															inode->block_pointers[3],inode->block_pointers[4]));
	//DEBUG(("- indirectBlock = %d\n", inode->indirect_block));
	DEBUG(("----------------------------------\n"));
}

