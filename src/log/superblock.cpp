#include "flash.h"
#include "segment.h"
#include "json.hpp"

SuperBlock::SuperBlock(unsigned int no_of_segments, unsigned int blocks_per_segment, unsigned int sectors_per_block, unsigned int blocks, unsigned int wear_limit){
    //Flash level info
    segmentCount = no_of_segments;
    blockCount = blocks;
    sectorCount = sectors_per_block * blocks;
    wearLimit = wear_limit;
    usedSegments = 1; //superblock uses one segment when initialized for the first time.

    //Segment level info
    blocksPerSegment = blocks_per_segment;
    sectorsPerSegment = sectors_per_block * blocks_per_segment;
    bytesPerSegment = FLASH_SECTOR_SIZE * sectors_per_block * blocks_per_segment;

    //Block level info
    sectorsPerBlock = sectors_per_block;
    bytesPerBlock = FLASH_SECTOR_SIZE * sectors_per_block;

    //Sector level info
    bytesPerSector = FLASH_SECTOR_SIZE;
}

void to_json(nlohmann::json& j, const SuperBlock& p) {
        j = nlohmann::json{{"segmentCount", p.segmentCount}, 
        {"blockCount", p.blockCount},
        {"sectorCount", p.sectorCount},
        {"wearLimit", p.wearLimit},
        {"usedSegments", p.usedSegments},

        {"blocksPerSegment", p.blocksPerSegment},
        {"sectorsPerSegment", p.sectorsPerSegment},
        {"bytesPerSegment", p.bytesPerSegment},

        {"sectorsPerBlock", p.sectorsPerBlock},
        {"bytesPerBlock", p.bytesPerBlock},

        {"bytesPerSector", p.bytesPerSector}
        };
}

//Unused anywhere, required as a place holder for json parsing.
void from_json(const nlohmann::json& j, SuperBlock& p) {
        j.at("segmentCount").get_to(p.segmentCount);
        j.at("blockCount").get_to(p.blockCount);
}