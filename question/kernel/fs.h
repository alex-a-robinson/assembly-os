#ifndef __FS_H
#define __FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "disk.h"
#include "bitmap.h"

// Inode constatnts
#define MAX_INODE_BLOCKS 6

// Super Block constants
#define MAX_NUMBER_OF_BLOCKS 65536
#define MIN_BLOCK_LENGTH 16
#define MAX_BLOCK_BITMAP_SIZE ((int) (MAX_NUMBER_OF_BLOCKS) / (BITS_PER_WORD))
#define MAX_NUMBER_OF_INODES ((int) (MAX_NUMBER_OF_BLOCKS) / (MIN_BLOCK_LENGTH))
#define MAX_INODE_BITMAP_SIZE ((int) (MAX_NUMBER_OF_INODES) / (BITS_PER_WORD))

// inode types
#define INODE_UNALLOCATED ( 0 )
#define INODE_FILE        ( 1 )
#define INODE_DIRECTORY   ( 2 )
#define INODE_DEVICE      ( 3 )

// directory constants
#define MAX_PATH_LENGTH 1024
#define MAX_FILE_NAME_LENGTH 100
#define MAX_FILES_IN_DIRECTORY 10

// File descriptors constants
#define MAX_OPEN_FILES 10
#define READ 0
#define WRITE 1
#define READ_WRITE 2 
#define READ_GLOBAL 3 // Anyone can read
#define WRITE_GLOBAL 4 // Anyone can write


typedef struct {
    int disk_block_num;
    int disk_block_length;
    int inode_num;
    uint32_t inode_start;
    uint32_t data_block_start;
    uint32_t free_block_bitmap[MAX_BLOCK_BITMAP_SIZE];
    uint32_t free_inode_bitmap[MAX_INODE_BITMAP_SIZE]; // inode table?
} superblock_t;

typedef struct {
    int id;
    int type;
    uint32_t size;
    uint32_t blocks_allocated;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t blocks[MAX_INODE_BLOCKS];
} inode_t;

typedef struct {
    int inode_id;
    char filename[MAX_FILE_NAME_LENGTH];
} file_link_t;

typedef struct {
    int files_count;
    file_link_t links[MAX_FILES_IN_DIRECTORY];
} directory_t;

typedef struct {
    int id;
    int flags;
    int inode_id;
} file_descriptor_t;

typedef struct {
    int count;
    file_descriptor_t open[MAX_OPEN_FILES];
} file_descriptor_table_t;

#endif
