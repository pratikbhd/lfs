#include <memory>
#include <iostream>
#include "segment.h"

Segment::Segment(unsigned int bytes_per_segment, unsigned int segmentNumber):bytesPerSegment(bytes_per_segment), segmentNumber(segmentNumber){
        data = std::make_shared<char>(bytes_per_segment);
        std::cout << "Segment Cached : " << segmentNumber << std::endl;
}

unsigned int Segment::GetSegmentNumber(){
        return segmentNumber;
}

Checkpoint::Checkpoint(log_address address, unsigned int time): address(address), time(time){

}


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