#ifndef __FS_H
#define __FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "bitmap.h"
#include "disk.h"

// Inode constatnts
#define MAX_INODE_BLOCKS 6

// Super Block constants
// TODO this should be 65536 as not using all the blocks
// TODO max number of inodes should exclude blocks used for super block
// TODO we don't use the blocks used by the inodes!
#define MAX_NUMBER_OF_BLOCKS 16384
#define MIN_BLOCK_LENGTH 16
#define MAX_BLOCK_LEN 1024
#define MAX_BLOCK_BITMAP_SIZE ((int) (MAX_NUMBER_OF_BLOCKS) / (BITS_PER_WORD))
//#define MAX_NUMBER_OF_INODES ((int) (MAX_NUMBER_OF_BLOCKS) / (MIN_BLOCK_LENGTH)) TODO
#define MAX_NUMBER_OF_INODES ( 10 )
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
    int size;
    int blocks_allocated;
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

/******************************************************
* Functions
******************************************************/

int disk_write(int block_len, uint32_t a, const uint8_t* x, int n);
int disk_read(int block_len, uint32_t a, const uint8_t* x, int n);

uint32_t blocks_occupied(int block_size, int bytes);
void allocate_block(superblock_t* superblock, uint32_t addr);
void unallocate_block(superblock_t* superblock, uint32_t addr);
uint32_t free_data_block(superblock_t* superblock);
int read_sequential_blocks(superblock_t* superblock, int block_size, uint32_t first_block_addr, uint8_t* data, int bytes);
int write_sequential_blocks(superblock_t* superblock, int block_size, uint32_t first_block_addr, uint8_t* data, int bytes);

int read_superblock(superblock_t* superblock);
int valid_superblock(superblock_t* superblock);
int write_superblock(superblock_t* superblock);
void new_superblock(superblock_t* superblock);

int path_to_inode(superblock_t* superblock, directory_t* dir, char* path);
int fdid_to_inode(superblock_t* superblock, file_descriptor_table_t* fdtable, int file_descriptor_id, inode_t* inode);
file_descriptor_t* fdid_to_fd(file_descriptor_table_t* fdtable, int file_descriptor_id);

uint32_t id_to_addr(superblock_t* superblock, int id);
void new_inode(int id, inode_t* inode);
int read_inode(superblock_t* superblock, int id, inode_t* inode);
int write_inode(superblock_t* superblock, inode_t* inode);
int free_inode(superblock_t* superblock, inode_t* inode);
int create_inode_type(superblock_t* superblock, int inode_type);
int delete_inode(superblock_t* superblock, inode_t* inode);
int read_from_inode(superblock_t* superblock, inode_t* inode, uint32_t* file_pointer, uint8_t* bytes, int data_size);
int write_to_inode(superblock_t* superblock, inode_t* inode, uint32_t* file_pointer, uint8_t* bytes, int data_size);

int write_dir(superblock_t* superblock, directory_t* dir);
int read_dir(superblock_t* superblock, inode_t* inode, directory_t* dir);
int create_directory(superblock_t* superblock, directory_t* parent_dir, char* filename, directory_t* dir);
//int delete_dir(superblock_t* superblock, inode_t* parent_dir_inode, char* filename);
//int delete_file_link(directory_t* dir, char* filename);
int directory_lookup(directory_t* dir_inode, char* filename);

int add_fd(superblock_t* superblock, file_descriptor_table_t* fdtable, int inode_id, int flags);


//int delete_file(superblock_t* superblock, inode_t* dir_inode, char* filename);
int create_file(superblock_t* superblock, directory_t* dir, char* filename, int type);
int open_file(superblock_t* superblock, file_descriptor_table_t* fdtable, directory_t* dir, char* path, int flags);
int close_file(superblock_t* superblock, file_descriptor_table_t* fdtable, int file_descriptor_id);

int init_inodes(superblock_t* superblock);
int init_disk(superblock_t* superblock, directory_t* root_dir);
int create_root_directory(superblock_t* superblock, directory_t* root_dir);
int read_root_dir(superblock_t* superblock, directory_t* dir);
int create_io_devices(superblock_t* superblock, file_descriptor_table_t* fdtable, directory_t* root_dir);

#endif
