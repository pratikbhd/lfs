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