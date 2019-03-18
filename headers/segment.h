#pragma once
#include <string>
#include "flash.h"
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
        Flash flash;
        unsigned int bytesPerSegment;
        unsigned int sectorsPerSegment;
        unsigned int segmentNumber;
        bool loaded = false;
        bool active = false;
    public:
        char *data;
        Segment(Flash flash, unsigned int bytes_per_segment, unsigned int sectors_per_segment, bool active_segment);
        Segment() = default;
        ~Segment();
        //Load a segment to memory.
        void Load(unsigned int segment_number);
        //Flush a loaded segment to flash.
        void Flush();
        //Force flush the segment to flash regardless of whether the segment has been loaded or not.
        void ForceFlush();
        //Erase the copy of the loaded segment in the flash.
        void Erase();
        unsigned int GetSegmentNumber();
};

//     char* cstring; // raw pointer used as a handle to a dynamically-allocated memory block
//     rule_of_three(const char* s, std::size_t n) // to avoid counting twice
//     : cstring(new char[n]) // allocate
//     {
//         std::memcpy(cstring, s, n); // populate
//     }
//  public:
    // rule_of_three(const char* s = "")
    // : rule_of_three(s, std::strlen(s) + 1)
    // {}
    // ~rule_of_three()
    // {
    //     delete[] cstring;  // deallocate
    // }
    // rule_of_three(const rule_of_three& other) // copy constructor
    // : rule_of_three(other.cstring)
    // {}
    // rule_of_three& operator=(rule_of_three other) // copy assignment
    // {
    //     std::swap(cstring, other.cstring);
    //     return *this;
    // }

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
        unsigned int inum;
        char     fileType;
        size_t   fileSize;
        unsigned int hardLinkCount;
        log_address block_pointers[4];
        log_address indirect_block;
        Inode() = default;
};

void to_json(nlohmann::json& j, const Inode& p);
void from_json(const nlohmann::json& j, Inode& p);

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

void to_json(nlohmann::json& j, const Checkpoint& p);
void from_json(const nlohmann::json& j, Checkpoint& p);

/**
 * Segment Summary Record
 */
struct block_usage {
    unsigned int inum;
    char use;
};

void to_json(nlohmann::json& j, const block_usage& p);

void from_json(const nlohmann::json& j, block_usage& p);