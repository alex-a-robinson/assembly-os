#ifndef __FS_H
#define __FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// TODO Should these be hard coded?
#define DISK_BLOCK_NUM 65536
#define DISK_BLOCK_LEN 16

// Inode constatnts
#define MAX_INODE_BLOCKS 6

// Super Block constants
#define NUM_OF_INODES ((int) (DISK_BLOCK_NUM) / (MAX_INODE_BLOCKS))

// inode types
#define INODE_UNALLOCATED 0
#define INODE_FILE 1
#define INODE_DIRECTORY 2

typedef struct {
    int disk_block_num;
    int disk_block_length;
    int inode_num;
    void* inode_start;
    void* data_block_start;
} superblock_t;

typedef struct {
    int id;
    int type;
    uint32_t size;
    uint32_t blocks_allocated;
    uint32_t creation_time;
    uint32_t modification_time;
    void* blocks[MAX_INODE_BLOCKS];
} inode_t;

#endif
