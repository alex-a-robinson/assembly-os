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
  - Write root files
*/

// Returns ceil of number of blocks bytes bytes occupies
uint32_t blocks_occupied(int block_size, int bytes) {
    return (block_size + bytes - 1) / block_size;
}

// Read superblock from disk
int read_superblock(superblock_t* superblock) {
    return disk_rd(0, superblock, sizeof(superblock_t));
}

// Returns 1 if superblock is valid else 0
int valid_superblock(superblock_t* superblock) {
    int block_num = disk_get_block_num();
    return (superblock->disk_block_num == block_num);
}

// Write a superblock to disk
int write_superblock(superblock_t* superblock) {
    return disk_wr(0, superbock, sizeof(superblock_t));
}

// Create a new superblock
void new_superblock(superblock_t* superblock) {
    memset(superblock, 0, sizeof(superblock_t));

    int block_num = disk_get_block_num();
    int block_len = disk_get_block_len();
    int inode_num = (int) block_num / MAX_INODE_BLOCKS;

    // Block num where inodes & data blocks start
    uint32_t inode_start = blocks_occupied(block_len, sizeof(superblock_t));
    uint32_t data_block_start = blocks_occupied(block_len, (sizeof(inode_t)*inode_num)) + inode_start;

    superblock->disk_block_num = block_num;
    superblock->disk_block_length = block_len;
    superblock->inode_num = inode_num;
    superblock->inode_start = inode_start;
    superblock->data_block_start = data_block_start;

    return;
}

// Initiate the disk for the first time
void init_disk(superblock_t* superblock) {
    int status = 0;
    new_superblock(superblock);
    status |= write_superblock(superblock);
    status |= write_inodes(superblock);
    status |= write_root_files(superblock);
    return status;
}

int write_inodes(superblock_t* superblock) {
    for (int i=0; i<superblock->inode_num; i++) {

    }
}

// Create an empty inode
void new_inode(int id, inode_t* inode) {
    memset(inode, 0, sizeof(inode_t));
    inode->id = id;
    return;
}

// Read inode by inode id
int read_inode(superblock_t* superblock, int id, inode_t* inode) {
    uint32_t addr = superblock->inode_start + id * blocks_occupied(superblock->disk_block_length, sizeof(inode_t));
    return disk_rd(addr, inode, sizeof(inode_t))
}

// NOTE This will fail if sizeof(inode_t) > block_size, same with superblock
// NOTE Currently giving one block to each inode
// Write inode
int write_inode(superblock_t* superblock, inode_t* inode) {
    int id = inode->id;
    uint32_t addr = superblock->inode_start + id * blocks_occupied(superblock->disk_block_length, sizeof(inode_t));
    return disk_wr(addr, inode, sizeof(inode_t));
}

void unallocate_inode(superblock_t* superblock, inode_t* inode) {
    // Unallocate data blocks
    for (int i=0; i<inode->blocks_allocated; i++) {
        unallocate_block(superblock, inode->blocks[i]);
    }
    int id = inode->id;
    new_inode(id, inode);
    return;
}

// Mark datablock as allocated
void allocate_block(superblock_t* superblock, unit32_t addr) {
    set_bit(superblock->free_block_bitmap, addr);
}

// Mark datablock as unallocated
void unallocate_block(superblock_t* superblock, unit32_t addr) {
    clear_bit(superblock->free_block_bitmap, addr);
}

// Returns next free block number or 0 if none
uint32_t free_data_block(superblock_t* superblock) {
    // Find 0 in superblock->free_block_bitmap, translate to data block pointer
    for (uint32_t addr=superblock->data_block_start; addr<superblock->disk_block_num; addr++) {
        if (get_bit(superblock->free_block_bitmap) == 0) {
            return addr;
        }
    }
    return 0;
}
