#include <iostream>
#include "segment.h"
#include "json.hpp"
#include "flash.h"
using json = nlohmann::json;

// json GetSuperBlock(Flash flash_handle){
//     char buffer[FLASH_SECTOR_SIZE+2];
//     SuperBlock *sb = (SuperBlock*) std::malloc(sizeof(SuperBlock)+1);
//     int res = Flash_Read(flash_handle, LOG_SUPERBLOCK_OFFSET, 1, buffer);
//     if (res) {
//         std::cout <<"SuperBlock READ FAIL" << std::endl;
//     }
//     std::memcpy(sb, buffer, sizeof(SuperBlock)+1);
//     //log_print_info(lInfo);
//     json json_sb = *sb;
//     return json_sb;
// }

json GetSuperBlock(Flash flash_handle){
    char buffer[FLASH_SECTOR_SIZE+2];
    SuperBlock *sb = new SuperBlock();
    int res = Flash_Read(flash_handle, LOG_SUPERBLOCK_OFFSET, 1, buffer);
    if (res) {
        std::cout <<"SuperBlock READ FAIL" << std::endl;
    }
    std::memcpy(sb, buffer, sizeof(SuperBlock)+1);
    //log_print_info(lInfo);
    json json_sb = *sb;
    return json_sb;
}

int main (int argc, char **argv)
{
    if (argc != 3) {
        std::cout << "usage: lfsck flashFile [log, file]\nEXITING\n";
        exit(EXIT_FAILURE);
    }
        
    json status;
    unsigned int blocks;
    auto flash = Flash_Open(argv[1], 0, &blocks);

    if(strcmp(argv[2], "log")==0){
        status["superblock"] = GetSuperBlock(flash);
    
    }else if(strcmp(argv[2], "file")==0)
        //TODO
        ;
    else
        printf("{}\n");

    std::string result = status.dump();
    std::cout << result << std::endl;
    exit(EXIT_SUCCESS);
}

