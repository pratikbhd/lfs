#include <iostream>
#include <fstream>
#include "segment.h"
#include "file.hpp"
#include "json.hpp"
#include "flash.h"
using json = nlohmann::json;

json GetSuperBlock(Log *l){
    json json_sb = (*l).super_block;
    return json_sb;
}

json GetSegmentSummaryBlocks(Log *l){
    json segments;
    for (unsigned int i=1; i < (*l).super_block.segmentCount; i++) {
        json blocks;
        for (unsigned int j=0; j < (*l).super_block.blocksPerSegment; j++) {
            block_usage b = (*l).GetBlockUsage((*l).GetLogAddress(i,j));
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

json GetiFile(File *f){
    json json_i = (*f).iFile;
    return json_i;
}

json GetIndirectBlocks(File *f, Inode in){
    json indirect;
    for(int i= 4; i < (*f).log.super_block.blocksPerSegment; i++) {
        log_address la = (*f).GetLogAddress(in, i);
        indirect.push_back(json{{"block", i}, 
        {"segmentNumber", la.segmentNumber},
        {"blockOffset", la.blockOffset}
        });
    }
    return indirect;
}

int main (int argc, char **argv)
{
    if (argc != 3) {
        std::cout << "usage: lfsck flashFile [log, file]\nEXITING\n";
        exit(EXIT_FAILURE);
    }
        
    json status;
    inputState state = inputState();
    state.lfsFile = argv[1];
    if(strcmp(argv[2], "log")==0){
        File file = File(state);
        status["superblock"] = GetSuperBlock(&file.log);
        status["segment_summary_block"] = GetSegmentSummaryBlocks(&file.log);
        status["checkpoints"] = GetCheckpoints(&file.log);
        status["iFile"] = GetiFile(&file);
    } else if(strcmp(argv[2], "file")==0){
        File file = File(state);
        status["superblock"] = GetSuperBlock(&file.log);
        status["checkpoints"] = GetCheckpoints(&file.log);
        status["iFile"] = json{{"inode", GetiFile(&file)}, {"blk",GetIndirectBlocks(&file, file.iFile)}};

        int i = 0;
        json inodeJsons;
        while(file.GetLogAddress(file.iFile, i).segmentNumber != 0) {
            Inode buffer[file.log.super_block.bytesPerBlock];
            log_address la = file.GetLogAddress(file.iFile, i);
            file.log.Read(la, file.log.super_block.bytesPerBlock, (char *)buffer);
            int j = 0;
            for(; (j * sizeof(Inode)) < file.log.super_block.bytesPerBlock; j++) {
                Inode in = buffer[j];
                inodeJsons.push_back(json{{"inode", in}, {"blk",GetIndirectBlocks(&file, in)}});
            }
            i++;
        }

        status["inodes"] = inodeJsons;
    }
    else
        printf("{}\n");

    std::string result = status.dump(4);

    std::string mode(argv[2]);
    std::ofstream jsonFile("lfs-"+mode+".json", std::ios_base::trunc);
    jsonFile << result;
    jsonFile.close();
    std::cout << result << std::endl;
    std::cout<< "Json snapshot complete!" << std::endl << "Press ENTER key to quit!";
    std::cin.get();
}

