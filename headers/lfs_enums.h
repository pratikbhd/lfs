#pragma once

enum class usage {
    FREE = 0,
    INUSE = 1
};

enum class reserved_inum {
    NOINUM = 0,
    IFILE = 1,
    ROOT = 2 
};

enum class fileTypes {
    NO_FILE = 0,
    PLAIN_FILE = 1,
    DIRECTORY = 2,
    SYM_LINK = 3,
    IFILE = 5
};

enum class fileLength {
    LENGTH = 256
};