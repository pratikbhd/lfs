#include "segment.h"

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