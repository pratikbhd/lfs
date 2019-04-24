#include "file.hpp"

class Cleaner {
    public:
    Cleaner(char* lfsFile);
	File file;

    void UpdateInode(Inode in, log_address before, log_address after);
    bool MergeSegments(int *segments);
    Inode Cleaner::GetInode(int inum);
};