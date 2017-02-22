#include "fs.h"

/*
TODO:
  - Implement Data blocks
  - Implement system calls
    - open, read, write, close
    - fstat = stat with file descriptor id
    - stat = file info from path
  - Implement cat and wc and load a program into memroy
  - Write root files
  - Read and write a byte at a time with inodes
*/

// NOTE simplification, this would be a mount table
superblock_t* mounted = NULL;

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

// Create and write empty inodes, returns 0 for success, -1 for failure
int write_inodes(superblock_t* superblock) {
    for (int i=0; i<superblock->inode_num; i++) {
        inode_t* inode;
        new_inode(i, inode);
        if (write_inode(superblock, inode) < 0) {
            return -1;
        }
    }
    return 0;
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
    for (uint32_t addr=superblock->data_block_start; addr<superblock->disk_block_num; addr++) {
        if (get_bit(superblock->free_block_bitmap, addr) == 0) {
            return addr;
        }
    }
    return 0;
}

// Mark inode as allocated
void allocate_inode(superblock_t* superblock, inode_t* inode) {
    // Mark allocated in superblock
    set_bit(superblock->free_inode_bitmap, inode->id);
}

// Unallocate an inode & its associated data blocks
void unallocate_inode(superblock_t* superblock, inode_t* inode) {
    // Unallocate data blocks
    for (int i=0; i<inode->blocks_allocated; i++) {
        unallocate_block(superblock, inode->blocks[i]);
    }

    // Mark unallocated in superblock inode bitmap
    clear_bit(superblock->free_inode_bitmap, inode->id);

    // Reset inode
    new_inode(inode->id, inode);
}

// Find the next free inode
int free_inode(superblock_t* superblock, inode_t* inode) {
    for (uint32_t id=0; id<superblock->inode_num; id++) {
        if (get_bit(superblock->free_inode_bitmap, id) == 0) {
            return read_inode(superblock, id, inode);
        }
    }
    return -1;
}

// This should be done with file handlers instead...
// int read_from_inode(inode_t* inode) {
//     uint_t data_size = inode->size;
//     uint8_t* data = malloc(sizeof(data_size));
//
//     for (int i=0; i < inode->blocks_allocated; i++) {
//         // Retrive bytes from the block,
//         if (disk_rd(inode->blocks[i], data, data_size % superblock->disk_block_length) < 0) {
//             return -1;
//         }
//         data_size -= superblock->disk_block_length; // Decrease number of blocks we're retriving for each block
//     }
//     return 0;
// }
