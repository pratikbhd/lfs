enum class usage {
    FREE = 0,
    INUSE = 1
};

enum class reserved_inum {
    NOINUM = 0,
    IFILE = 1
};

enum class fileTypes {
    NO_FILE = 0,
    PLAIN_FILE = 1,
    DIRECTORY = 2,
    SYM_LINK = 3,
    IFILE = 5
}