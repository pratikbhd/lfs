#include <iostream>
#include "segment.h"
#include "log.h"
#include "json.hpp"
#include "flash.h"
using json = nlohmann::json;

json GetSuperBlock(Log *l){
    (*l).GetSuperBlock();
    json json_sb = (*l).super_block;
    return json_sb;
}

json GetSegmentSummaryBlocks(Log *l){
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

json GetCheckpoints(Log *l){
    /* load checkpoints */
    (*l).cp1 = (*l).GetCheckpoint(LOG_CP1_OFFSET);
    (*l).cp2 = (*l).GetCheckpoint(LOG_CP2_OFFSET);

    json json_cp;
    json_cp.push_back((*l).cp1);
    json_cp.push_back((*l).cp2);
    return json_cp;
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
        status["superblock"] = GetSuperBlock(&log);
        log.InitializeCache();
        status["segment_summary_block"] = GetSegmentSummaryBlocks(&log);
        status["checkpoints"] = GetCheckpoints(&log);
        Flash_Close(log.flash);
    } else if(strcmp(argv[2], "file")==0)
        //TODO
        ;
    else
        printf("{}\n");

    std::string result = status.dump(4);
    std::cout << result << std::endl;
    std::cout<< "Json snapshot complete!" << std::endl << "Press ENTER key to quit!";
    std::cin.get();
}

