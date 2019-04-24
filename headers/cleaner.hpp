#include "file.hpp"

class Cleaner {
    private:
    	File file;
        inputState state;
        void UpdateInode(Inode in, log_address before, log_address after);
        bool MergeSegments(int *segments);
        bool CleanBlocks();
        Inode GetInode(int inum);
    public:
        Cleaner(inputState state);
        bool Clean();
};