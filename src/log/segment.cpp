#include <memory>
#include <iostream>
#include "segment.h"
#include "flash.h"
#include "color.hpp"

Segment::Segment(Flash flash, unsigned int bytes_per_segment, unsigned int sectors_per_segment, bool active_segment): 
flash(flash), bytesPerSegment(bytes_per_segment), sectorsPerSegment(sectors_per_segment), active(active_segment), 
data(new char[bytes_per_segment + 1]){

}

//Explicity we have to delete all pointer members of this class to avoid memory leaks.
Segment::~Segment(){
        std::cout << "[Segment] Segment Deleted : " << segmentNumber << std::endl;
        delete[] data;  // deallocate
}

unsigned int Segment::GetSegmentNumber(){
        return segmentNumber;
}

void Segment::Load(unsigned int segment_number) {   
    segmentNumber = segment_number;
    unsigned int sector_num = segmentNumber * sectorsPerSegment;
    delete[] data;
    data = new char[bytesPerSegment + 1];

    char buffer[bytesPerSegment + 1];

    int res = Flash_Read(flash, 
                sector_num, 
                sectorsPerSegment, 
                buffer
              );
    if(res){
        Color::Modifier red(Color::FG_RED);
        Color::Modifier def(Color::FG_DEFAULT);
        std::cout << red << "[Segment] Segment::Load() READ FAIL!" << def << std::endl;
    } else {
        std::memcpy(data, buffer, bytesPerSegment + 1);
        loaded = true;
        std::cout << "[Segment] Segment::Load() Segment Cached : " << segmentNumber << std::endl;   
    }
}

void Segment::Erase(){
    unsigned int block_number = (segmentNumber*sectorsPerSegment)/FLASH_SECTORS_PER_BLOCK;
    int res = Flash_Erase(flash, block_number, sectorsPerSegment/FLASH_SECTORS_PER_BLOCK);
    if (res){
        Color::Modifier red(Color::FG_RED);
        Color::Modifier def(Color::FG_DEFAULT);
        std::cout << red << "[Segment] Segment::Erase() Erasing the segment failed" <<def<< std::endl;
    }
}

void Segment::ForceFlush(){
    loaded = true;
    Flush();
}

bool Segment::IsLoaded(){
    return loaded;
}

void Segment::Flush(){
    //only an active segment is written back to log. This segment should be the log end.
    if(!active) return;
    //A Segment is not allocated to a valid flash segment.
    if(!loaded) return;

    Erase();
    unsigned int sector_number = segmentNumber*sectorsPerSegment;
    
    int res = Flash_Write(flash,
                 sector_number,
                 sectorsPerSegment,
                 data
               );
    if(res){
        Color::Modifier red(Color::FG_RED);
        Color::Modifier def(Color::FG_DEFAULT);
        std::cout << red << "[Segment] Segment::Flush() Flushing the Segment :" << segmentNumber << "to flash failed!" << def<< std::endl;
    }

    loaded = false;
}