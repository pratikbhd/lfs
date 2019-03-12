#pragma once
#include <string>
#include "json.hpp"

/**
 * log offsets
 */
#define LOG_SUPERBLOCK_OFFSET 0
#define LOG_CP1_OFFSET 1
#define LOG_CP2_OFFSET 2
#define LOG_IFILE_OFFSET 3

class Segment {
    private:
        long segment_size_bytes;
        std::string data;
    public:
        int size;
        Segment(std::string data, int size);
        Segment() = default;
        char* GetData();
};

class SuperBlock {
    public:
        unsigned int segmentCount;       // number of segments in LFS
        unsigned int blockCount;         // number of block in LFS
        unsigned int sectorCount;        // number of sectors in LFS
        unsigned int blocksPerSegment;   // blocks
        unsigned int sectorsPerSegment;  // sectors
        unsigned int bytesPerSegment;    // bytes
        unsigned int sectorsPerBlock;    // segments
        unsigned int bytesPerBlock;      // bytes
        unsigned int bytesPerSector;     // bytes
        unsigned int wearLimit;          // wear limit
        //char *segmentUsage;     // segment usage
        unsigned int usedSegments;       // available segments

        SuperBlock(unsigned int no_of_segments, unsigned int blocks_per_segment, unsigned int sectors_per_block, unsigned int blocks, unsigned int wear_limit);
        SuperBlock() = default;
};

void to_json(nlohmann::json& j, const SuperBlock& p);

void from_json(const nlohmann::json& j, SuperBlock& p);

/**
 * Internal log address
 */
struct log_address {
    unsigned int segmentNumber;
    unsigned int blockOffset;
};

/**
 * The inode structure used by the file system.
 * 
 * Members:
 * -- int inum: 
 *        a unique identifier for each inode. The ifile inum has inode -1. Every other inode is non-negative and
 *        acts as an index for the inode in the ifile
 *
 * -- char fileType: 
 *        The type of file. This provides information about what can be done to/with a file.
 *            NO_FILE if the inode is not being used.
 *            PLAIN_FILE for regular files containing unstructered data.
 *            DIRECTORY if the inode is associated with a directory.
 *            SYM_LINK if the inode is associated with a symbolic link
 *            IFILE if this is the inode of the ifile, with inum -1.
 *
 * -- size_t fileSize: 
 *        The total length of the file in bytes. This is non-negative and bounded by FILE_MAX_SIZE.
 *
 * -- uint32_t hardLinkCount:
 *        The number of hardlinks to the inodes file. Non-negative. If 0, then this inode does not have a file.
 *  
 * -- uint32_t block_pointers[4]: 
 *        An array of pointers to the first 4 blocks of the file in the log.
 *
 * -- uint32_t indirectBlock:
 *        A pointer to a block in the log, which stores more pointers to the file. 
 */
class Inode {
    public:
        unsigned int      inum;
        char     fileType;
        size_t   fileSize;
        unsigned int hardLinkCount;
        log_address block_pointers[4];
        log_address indirect_block;
        Inode() = default;
};

/**
 * Checkpoint.
 */
class Checkpoint {
    public:
        log_address address;
        unsigned int time;
        Checkpoint(log_address address, unsigned int time);
        Checkpoint() = default;
};

/**
 * Segment Summary Record
 */
struct block_usage {
    unsigned int inum;
    char use;
};

enum class usage {
    FREE,
    INUSE,
    NOINUM = -1
};