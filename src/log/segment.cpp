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

Checkpoint::Checkpoint(log_address address, unsigned int time): address(address), time(time){

}