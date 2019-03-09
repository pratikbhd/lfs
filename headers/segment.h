#include <string>

/**
 * log offsets
 */
#define LOG_INFO_OFFSET 0
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
        //int sectorCount;        // number of sectors in LFS
        // int blocksPerSegment;   // blocks
        // int sectorsPerSegment;  // sectors
        // int bytesPerSegment;    // bytes
        // int sectorsPerBlock;    // segments
        // int bytesPerBlock;      // bytes
        // int bytesPerSector;     // bytes
        // int wearLimit;          // wear limit
        //char *segmentUsage;     // segment usage
        // int usedSegments;       // available segments
        SuperBlock() = default;
};