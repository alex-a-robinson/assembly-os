#include "fs.h"

/*
TODO:
  - Check for existance of super block, create if dosen't exist
  - implement read by direct inode number first
  - Create super block layout
    - size of disk, number of inodes, number of data blocks, where inodes start, where data blocks start
  - Implement inode structure
    - inode number
    - TYPE = "FILE" or "Directory" or "UNALLOCATED"
    - size = file size in bytes
    - number of blocks allocated
    - creation time, modification time, ...
    - 6 Direct Block Pointers which point to specific data blocks
  - Implement Data blocks
  - In memory inode lookup table
  - Implement system calls
    - open, read, write, close
    - fstat = stat with file descriptor id
    - stat = file info from path
  - Implement cat and wc and load a program into memroy
*/
