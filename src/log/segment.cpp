#pragma once
#include "flash.h"
#include "segment.h"
#include "json.hpp"

Segment::Segment(std::string data, int size):data(data), size(size){

}

void to_json(nlohmann::json& j, const SuperBlock& p) {
        j = nlohmann::json{{"segmentCount", p.segmentCount}, 
        {"blockCount", p.blockCount}};
}

void from_json(const nlohmann::json& j, SuperBlock& p) {
        j.at("segmentCount").get_to(p.segmentCount);
        j.at("blockCount").get_to(p.blockCount);
}

SuperBlock::SuperBlock(int no_of_segments, int blocks_per_segment, int blocks, int wear_limit){
    segmentCount = no_of_segments;
    blockCount = blocks;
    sectorCount = FLASH_SECTORS_PER_BLOCK * blocks;
    blocksPerSegment = blocks_per_segment;
    sectorsPerSegment = FLASH_SECTORS_PER_BLOCK * blocks_per_segment;
    bytesPerSegment = FLASH_SECTOR_SIZE * FLASH_SECTORS_PER_BLOCK * blocks_per_segment;
    sectorsPerBlock = FLASH_SECTORS_PER_BLOCK;
    bytesPerBlock = FLASH_SECTOR_SIZE * FLASH_SECTORS_PER_BLOCK;
    bytesPerSector = FLASH_SECTOR_SIZE;
    wearLimit = wear_limit;
    usedSegments = 1; //superblock uses one segment when initialized for the first time.
}
