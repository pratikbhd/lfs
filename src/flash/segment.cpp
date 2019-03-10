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
