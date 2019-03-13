#include <memory>
#include <iostream>
#include "segment.h"
#include "flash.h"

Segment::Segment(Flash flash, unsigned int bytes_per_segment, unsigned int sectors_per_segment): 
flash(flash), bytesPerSegment(bytes_per_segment), sectorsPerSegment(sectors_per_segment), data(new char[bytes_per_segment + 10]){

}

Segment::~Segment(){
        delete[] data;  // deallocate
}

unsigned int Segment::GetSegmentNumber(){
        return segmentNumber;
}

void Segment::Load(unsigned int segment_number) {   
    segmentNumber = segment_number;
    unsigned int sector_num = segmentNumber * sectorsPerSegment;
    //std::memset(data, 0, bytesPerSegment);
    delete[] data;
    data = new char[bytesPerSegment + 10];

    char buffer[bytesPerSegment +1];

    int res = Flash_Read(flash, 
                sector_num, 
                sectorsPerSegment, 
                buffer
              );
    if(res)
        std::cout << "READ FAIL!" << std::endl;

    std::memcpy(data, buffer, bytesPerSegment + 1);
    std::cout << "Segment Cached : " << segmentNumber << std::endl;
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