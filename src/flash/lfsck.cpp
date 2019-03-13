#include <iostream>
#include "segment.h"
#include "log.h"
#include "json.hpp"
#include "flash.h"
using json = nlohmann::json;

json GetSuperBlock(Flash flash_handle, Log *l){
    char buffer[FLASH_SECTOR_SIZE+2];
    (*l).super_block = SuperBlock();
    int res = Flash_Read(flash_handle, LOG_SUPERBLOCK_OFFSET, 1, buffer);
    if (res) {
        std::cout <<"SuperBlock READ FAIL" << std::endl;
    }
    std::memcpy(&(*l).super_block, buffer, sizeof(SuperBlock)+1);
    //log_print_info(lInfo);
    json json_sb = (*l).super_block;
    return json_sb;
}

json GetSegmentSummaryBlocks(Flash flash_handle, Log *l){
    json segments;
    for (unsigned int i=1; i < (*l).super_block.segmentCount; i++) {
        json blocks;
        for (unsigned int j=0; j < (*l).super_block.blocksPerSegment; j++) {
            block_usage b = (*l).GetBlockUsageRecord((*l).GetLogAddress(i,j));
            json bjson = b;
            blocks.push_back(bjson);
        }
        segments.push_back(blocks);
    }
    return segments;
}

int main (int argc, char **argv)
{
    if (argc != 3) {
        std::cout << "usage: lfsck flashFile [log, file]\nEXITING\n";
        exit(EXIT_FAILURE);
    }
        
    json status;
    if(strcmp(argv[2], "log")==0){
        Log log = Log();
        unsigned int blocks;
        log.flash = Flash_Open(argv[1], 0, &blocks);
        status["superblock"] = GetSuperBlock(log.flash, &log);
        status["segment_summary_block"] = GetSegmentSummaryBlocks(log.flash, &log);
        Flash_Close(log.flash);
    }else if(strcmp(argv[2], "file")==0)
        //TODO
        ;
    else
        printf("{}\n");

    std::string result = status.dump(4);
    std::cout << result << std::endl;
    std::cout<< "Json snapshot complete!" << std::endl << "Press ENTER key to quit!";
    std::cin.get();
}

