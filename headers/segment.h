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
        int segmentCount;       // number of segments in LFS
        int blockCount;         // number of block in LFS
        int sectorCount;        // number of sectors in LFS
        int blocksPerSegment;   // blocks
        int sectorsPerSegment;  // sectors
        int bytesPerSegment;    // bytes
        int sectorsPerBlock;    // segments
        int bytesPerBlock;      // bytes
        int bytesPerSector;     // bytes
        int wearLimit;          // wear limit
        //char *segmentUsage;     // segment usage
        int usedSegments;       // available segments

        SuperBlock(int no_of_segments, int blocks_per_segment, int blocks, int wear_limit);
        SuperBlock() = default;
};

void to_json(nlohmann::json& j, const SuperBlock& p);

void from_json(const nlohmann::json& j, SuperBlock& p);

/**
 * Internal log address
 */
struct log_address {
    int segmentNumber;
    int blockOffset;
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
    int      inum;
    char     fileType;
    size_t   fileSize;
    uint32_t hardLinkCount;
    log_address block_pointers[4];
    log_address indirect_block;
};

/**
 * Checkpoint reference.
 */
struct checkpoint {
    log_address address;
    char val;
};

/**
 * Segment Summary Record
 */
struct block_usage {
    int inum;
    char use;
};

enum class usage {
    FREE,
    INUSE,
    NOINUM = -1
};