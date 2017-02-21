#ifndef __FS_H
#define __FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MAX_INODE_BLOCKS 6

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
