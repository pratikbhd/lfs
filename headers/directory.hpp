#include <sys/types.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <errno.h>

#include "log.h"
#include "segment.h"

// The inum of the ifile will be 0 and that of the root will be 0 -- TODO: when will root be used?
// #define IFILE_INODE -1
// #define ROOT_INODE   0

// Use integers to specify the kind of file. -- TODO: THis can be changed later.

#define NO_FILE        0
#define PLAIN_FILE     1
#define DIRECTORY      2
#define SYM_LINK       3
#define IFILE          5

// TODO: change in phase 2
#define NAME_MAX_LENGTH 256
#define FILE_MAX_SIZE   BLOCK_SIZE * (4 + (lInfo->bytesPerBlock) / sizeof(log_addr))
#define INODES_MAX_NUM  FILE_MAX_SIZE / sizeof(Inode)
#define BLOCK_SIZE lInfo->bytesPerBlock

class Directory {
	public:
		/*
 		* Create and delete the directory structure
 		*/
		void *Directory_Initialize(struct fuse_conn_info*);

		/**
 		* Saves and releases all data about the File system upon dismount. Ultimately very little is done.
 		*/
		void Directory_Destroy(void *data);

		/**
		 * Creates a new directory with the path <*path>. The <mode> argument is ignored and all permissions are positive.
		 */
		int Directory_MakeDir(const char *path, mode_t mode);

		/**
		 * Reads <length> bytes into <buf> from the file pointed by <path> starting at <offset> in the file. This is just an interface
		 * to File_Read.
		 */
		int Directory_Read(const char *path, char *buf, size_t length, off_t offset,
	   			struct fuse_file_info *fi);

		/**
		 * Writes <length> bytes from <buf> to the file pointed by <path> starting at <offset> in the file. This is just an interface
		 * to File_Write.
		 */
		int Directory_Write(const char *path, const char *buf, size_t length, off_t offset,
	   					struct fuse_file_info *fi);

		/**
		 * Reads the contents of the directory pointed by <path> and places the names of all files in <buf>. The entries '.' and '..' are
		 * also placed in <buf>.
		 */
		int Directory_Readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
				off_t offset, struct fuse_file_info *fi);

		/**
		 * Adds the pair <file->inum, *fileName> to the directory with Inode <*dir>. The entry is added in lexicographical order
		 * with respect to <*fileName>.
		 */
		int Directory_AddEntry(Inode *dir, Inode *file, const char *fileName);
		
		/**
		 * Prints the value of each member of the inode argument.
		 */
		void printInode(Inode *);
		
		/**
		 * Writes the inode argument to the ifile. Returns 0 always.
		 */
		int writeInode(Inode *);
		
		/**
 		* Stores the Inode associated with <path> in the memory pointed by <inode>. If <path> does not lead to a file, then
 		* an error code is returned and <inode> points to undefined data.
 		*/
		int Directory_ReadPath(const char *path, Inode *inode);

		/**
		 * Creates a new Inode, and sets all of its members to a null value except for inum. This Inode is appended to the ifile.
		 * The new Inode will be pointed to by the <inode> pointer argument.
		 */
		int Directory_NewInode(Inode *inode);
		
		int Directory_ToggleInumIsUsed(int inum);
		
		int Directory_InumIsUsed(int inum);
		
		int Directory_CountInodes();
		/**
		 * Sets every value of <ino> to null except for the inum, then writes <ino> to the log.
		 */
		int Directory_ResetInode(Inode *ino);

		/**
		 * Returns a pointer to the inode with <inum>.
		 */
		Inode *Directory_GetInode(int inum);

#endif 
