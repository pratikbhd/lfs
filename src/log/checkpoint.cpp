#include "lfs_enums.h"
#include "segment.h"
#include <string>

Checkpoint::Checkpoint(log_address address, unsigned int time): address(address), time(time){

}

//Json handlers for Checkpoint class
void to_json(nlohmann::json& j, const Checkpoint& p) {
        j = nlohmann::json{{"segmentNumber", p.address.segmentNumber}, 
                {"blockOffset", p.address.blockOffset},
                {"time", p.time}
        };
}

void from_json(const nlohmann::json& j, Checkpoint& p) {
        j.at("segmentNumber").get_to(p.address.segmentNumber);
        j.at("blockOffset").get_to(p.address.blockOffset);
        j.at("time").get_to(p.time);
}

//Json handlers for block usage struct
void to_json(nlohmann::json& j, const block_usage& p) {
        std::string use = (p.use == static_cast<char>(usage::INUSE)) ? "INUSE" : "FREE";
        j = nlohmann::json{{"inum", p.inum}, 
        {"use", use}
        };
}

void from_json(const nlohmann::json& j, block_usage& p) {
        std::string use;
        j.at("inum").get_to(p.inum);
        j.at("use").get_to(use);
        p.use = static_cast<char>(use == "INUSE" ? usage::INUSE : usage::FREE);
}

//Json handlers for inode
void to_json(nlohmann::json& j, const Inode& p){
        j = nlohmann::json{{"inum", p.inum}, 
                {"fileSize", p.fileSize},
                {"fileType", p.fileType},
                {"hardLinkCount", p.hardLinkCount},
                {"block_0", std::to_string(p.block_pointers[0].segmentNumber) + " : " + std::to_string(p.block_pointers[0].blockOffset)},
                {"block_1", std::to_string(p.block_pointers[1].segmentNumber) + " : " + std::to_string(p.block_pointers[1].blockOffset)},
                {"block_2", std::to_string(p.block_pointers[2].segmentNumber) + " : " + std::to_string(p.block_pointers[2].blockOffset)},
                {"block_3", std::to_string(p.block_pointers[3].segmentNumber) + " : " + std::to_string(p.block_pointers[3].blockOffset)},
                {"indirect_block", std::to_string(p.indirect_block.segmentNumber) + " : " + std::to_string(p.indirect_block.blockOffset)},
        };
}

void from_json(const nlohmann::json& j, Inode& p){
        j.at("inum").get_to(p.inum);
}