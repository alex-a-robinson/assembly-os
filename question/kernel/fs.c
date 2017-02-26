#include "fs.h"

/******************************************************
* Better read/write
******************************************************/

int disk_write(int block_len, uint32_t a, const uint8_t* x, int n) {
    uint8_t data[MAX_BLOCK_LEN];
    memcpy(data, x, n);
    return disk_wr(a, data, block_len);
}

int disk_read(int block_len, uint32_t a, const uint8_t* x, int n) {
    uint8_t data[MAX_BLOCK_LEN];
    int status = disk_rd(a, data, block_len);
    memcpy(x, data, n);
    return status;
}

/******************************************************
* Block operations
******************************************************/

// Returns ceil of number of blocks bytes bytes occupies
uint32_t blocks_occupied(int block_size, int bytes) {
    return (block_size + bytes - 1) / block_size;
}

// Mark datablock as allocated
void allocate_block(superblock_t* superblock, uint32_t addr) {
    set_bit(superblock->free_block_bitmap, addr);
}

// Mark datablock as unallocated
void unallocate_block(superblock_t* superblock, uint32_t addr) {
    clear_bit(superblock->free_block_bitmap, addr);
}

// Returns next free block number or 0 if none
uint32_t free_data_block(superblock_t* superblock) {
    for (uint32_t addr=superblock->data_block_start; addr < (uint32_t)superblock->disk_block_num; addr++) {
        if (get_bit(superblock->free_block_bitmap, addr) == 0) {
            return addr;
        }
    }
    return 0;
}

int _rw_sequential_blocks(superblock_t* superblock, int block_size, uint32_t first_block_addr, uint8_t* _data, int bytes, int read) {
    uint8_t* data = _data; // Copy so we don't change the pointer TODO check

    uint32_t blocks = (uint32_t)blocks_occupied(block_size, bytes);
    int bytes_left = bytes;

    int bytes_to_read_or_write;
    for (uint32_t addr=first_block_addr; addr<first_block_addr+blocks; addr++) {
        if (bytes_left < block_size) { // If less then an entire block
            bytes_to_read_or_write = bytes_left;
        } else { // Else read/write the entire block
            bytes_to_read_or_write = block_size;
        }

        int status;
        if (read) {
            status = disk_read(block_size, addr, data, bytes_to_read_or_write);
        } else {
            // Mark allocated in superblock
            allocate_block(superblock, addr);
            status = disk_write(block_size, addr, data, bytes_to_read_or_write);
        }

        // Check for failure
        if (status < 0) {
            return -1;
        }

        // Update how many bytes left & Move pointer
        bytes_left -= bytes_to_read_or_write;
        data += bytes_to_read_or_write;

        // Check there is still data to read/write
        if (bytes_left <= 0) {
            break;
        }
    }

    return 0;
}

int read_sequential_blocks(superblock_t* superblock, int block_size, uint32_t first_block_addr, uint8_t* data, int bytes) {
    return _rw_sequential_blocks(superblock, block_size, first_block_addr, data, bytes, 1);
}

int write_sequential_blocks(superblock_t* superblock, int block_size, uint32_t first_block_addr, uint8_t* data, int bytes) {
    return _rw_sequential_blocks(superblock, block_size, first_block_addr, data, bytes, 0);
}

/******************************************************
* Super Block operations
******************************************************/

// Read superblock from disk
int read_superblock(superblock_t* superblock) {
    int block_len = disk_get_block_len();
    return read_sequential_blocks(superblock, block_len, 0, (uint8_t*)superblock, sizeof(superblock_t));
}

// Returns 1 if superblock is valid else 0
int valid_superblock(superblock_t* superblock) {
    int block_num = disk_get_block_num();
    return (superblock->disk_block_num == block_num);
}

// Write a superblock to disk
int write_superblock(superblock_t* superblock) {
    return write_sequential_blocks(superblock, superblock->disk_block_length, 0, (uint8_t*)superblock, sizeof(superblock_t));
}

// Create a new superblock
void new_superblock(superblock_t* superblock) {
    memset(superblock, 0, sizeof(superblock_t));

    int block_num = disk_get_block_num();
    int block_len = disk_get_block_len();
    int inode_num = (int) block_num / MAX_INODE_BLOCKS;

    if (inode_num > MAX_NUMBER_OF_INODES) {
        inode_num = MAX_NUMBER_OF_INODES;
    }

    // Block num where inodes & data blocks start
    uint32_t inode_start = blocks_occupied(block_len, sizeof(superblock_t));
    uint32_t data_block_start = blocks_occupied(block_len, sizeof(inode_t)) * inode_num + inode_start;

    superblock->disk_block_num = block_num;
    superblock->disk_block_length = block_len;
    superblock->inode_num = inode_num;
    superblock->inode_start = inode_start;
    superblock->data_block_start = data_block_start;

    return;
}

/******************************************************
* Misc operations
******************************************************/

// int dir_to_inode(superblock_t* superblock, directory_t* dir, inode_t* inode) {
//      int id = dir->links[0].inode_id; // ID of "." entry in dir
//      return read_inode(superblock, id, inode);
//  }

//  // Return a inode from a filename
// int filename_to_inode(superblock_t* superblock, directory_t* dir, char* filename, inode_t* inode) {
//      int inode_id = directory_lookup(dir, filename);
//
//      // Check we found the inode
//      if (inode_id < 0) {
//          return -1;
//      }
//
//      // Read the inode, check for errors
//      if (read_inode(superblock, inode_id, inode) < 0) {
//          return -1;
//      }
//
//      return 0;
//  }

// Return a directory from a filename
// int filename_to_dir(superblock_t* superblock, inode_t* parent_dir_inode, char* filename, directory_t* dir) {
//      inode_t dir_inode;
//      if (filename_to_inode(superblock, parent_dir_inode, filename, &dir_inode) < 0) {
//          return -1;
//      }
//
//      uint32_t file_pointer = 0;
//      if (read_from_inode(superblock, &dir_inode, &file_pointer, (uint8_t*)dir, sizeof(directory_t)) < 0) {
//          return -1;
//      }
//
//      return 0;
//  }

// Returns inode id from path
int path_to_inode_id(superblock_t* superblock, directory_t* dir, char* path) {
    inode_t inode;
    directory_t current_dir;
    memset(&current_dir, 0, sizeof(directory_t));
    memcpy(&current_dir, dir, sizeof(directory_t));

    // Split string on "/"
    char* part = strtok(path, "/");

    // Until path is empty & we have directories
    int inode_id = -1;
    while(part != NULL) {
        inode_id = directory_lookup(&current_dir, part);
        read_inode(superblock, inode_id, &inode);
        if (inode.type == INODE_DIRECTORY) {
            read_dir(superblock, &inode, &current_dir);
        } else {
            break;
        }
        part = strtok(path, "/");
    }

    // Error before we finished the path
    if (part != NULL) {
        return -1;
    }

    return inode_id;
}

file_descriptor_t* fdid_to_fd(file_descriptor_table_t* fdtable, int file_descriptor_id) {
    for (int i=0; i < fdtable->count; i++) {
        if (fdtable->open[i].id == file_descriptor_id) {
            return &fdtable->open[i];
        }
    }
    return NULL;
}

// Returns inode from a file descriptor id, returns 0 on success
int fdid_to_inode(superblock_t* superblock, file_descriptor_table_t* fdtable, int file_descriptor_id, inode_t* inode) {
    file_descriptor_t* fd = NULL;
    fd = fdid_to_fd(fdtable, file_descriptor_id);

    if (fd == 0) {
        return -1;
    }

    return read_inode(superblock, fd->id, inode);
}


/******************************************************
* Inode operations
******************************************************/

uint32_t id_to_addr(superblock_t* superblock, int id) {
    return superblock->inode_start + id * blocks_occupied(superblock->disk_block_length, sizeof(inode_t));
}

// Create an empty inode
void new_inode(int id, inode_t* inode) {
    memset(inode, 0, sizeof(inode_t));
    inode->id = id;
}

// Read inode by inode id
int read_inode(superblock_t* superblock, int id, inode_t* inode) {
    uint32_t addr = id_to_addr(superblock, id);
    return read_sequential_blocks(superblock, superblock->disk_block_length, addr, (uint8_t*)inode, sizeof(inode_t));
}

// NOTE This will fail if sizeof(inode_t) > block_size, same with superblock
// NOTE Currently giving one block to each inode
// Write inode
int write_inode(superblock_t* superblock, inode_t* inode) {
    uint32_t addr = id_to_addr(superblock, inode->id);
    return write_sequential_blocks(superblock, superblock->disk_block_length, addr, (uint8_t*)inode, sizeof(inode_t));
}

int free_inode_id(superblock_t* superblock) {
    for (int id=0; id<superblock->inode_num; id++) {
        if (get_bit(superblock->free_inode_bitmap, id) == 0) {
            return id;
        }
    }
    return -1;
}

// Find the next free inode
int free_inode(superblock_t* superblock, inode_t* inode) {
    int id = free_inode_id(superblock);
    return read_inode(superblock, id, inode);
}

// Creates an inode with a specific type and returns the inode id
int create_inode_type(superblock_t* superblock, int inode_type) {
    inode_t inode;
    if (free_inode(superblock, &inode) < 0) {
        return -1;
    }

    // Mark allocated in superblock
    set_bit(superblock->free_inode_bitmap, inode.id);

    inode.type = inode_type;
    inode.creation_time = 1; // TODO
    if (write_inode(superblock, &inode) < 0) {
        return -1;
    }

    return inode.id;
}

// Unallocate an inode & its associated data blocks
int delete_inode(superblock_t* superblock, inode_t* inode) {
    // Unallocate data blocks
    for (int i=0; i<inode->blocks_allocated; i++) {
        unallocate_block(superblock, inode->blocks[i]);
    }

    // Mark unallocated in superblock inode bitmap
    clear_bit(superblock->free_inode_bitmap, inode->id);

    // Reset inode
    new_inode(inode->id, inode);
    return write_inode(superblock, inode);
}


// Read/Write <size> bytes from <bytes> from file_pointer, returns 0 on success
int _rw_inode(superblock_t* superblock, inode_t* inode, uint32_t* file_pointer, uint8_t* _bytes, int data_size, int read) {
    uint8_t* bytes = _bytes; // Copy so we don't change the pointer TODO check

    // Check the data is small enough
    int num_of_blocks = blocks_occupied(superblock->disk_block_length, data_size);
    if (num_of_blocks > MAX_INODE_BLOCKS) {
        return -1;
    }

    // Update the blocks allocated / keep the old one
    if (num_of_blocks > inode->blocks_allocated) {
        inode->blocks_allocated = num_of_blocks;
    }

    int bytes_left = data_size;
    int block_id = (*file_pointer) / superblock->disk_block_length;
    int offset   = (*file_pointer) % superblock->disk_block_length;

    uint32_t addr;
    int first = 1;
    int bytes_in_block;
    int bytes_to_read_or_write;

    // TODO offsetting wont work needs differnt logic

    // For each avalible block
    for (int i=block_id; i<inode->blocks_allocated; i++) {
        // Find the block address
        if (read) {
            addr = inode->blocks[i];
        } else {
            addr = free_data_block(superblock);
        }

        // Offset only the first address
        if (first) {
            first = 0;
            addr += offset;
            bytes_in_block = superblock->disk_block_length - offset;
        } else {
            bytes_in_block = superblock->disk_block_length;
        }

        if (bytes_left < bytes_in_block) { // If fits all in one block
            bytes_to_read_or_write = bytes_left;
        } else { // Else read/write the entire block
            bytes_to_read_or_write = bytes_in_block;
        }

        int status;
        if (read) {
            status = disk_read(superblock->disk_block_length, addr, bytes, bytes_to_read_or_write);
        } else {
            // Mark allocated in superblock & inode
            inode->blocks[i] = addr;
            allocate_block(superblock, addr);
            status = disk_write(superblock->disk_block_length, addr, bytes, bytes_to_read_or_write);
        }

        // Check for failure
        if (status < 0) {
            return -1;
        }

        // Update how many bytes left & Move pointer
        bytes_left -= bytes_to_read_or_write;
        bytes += bytes_to_read_or_write;

        // Check there is still data to read/write
        if (bytes_left <= 0) {
            break;
        }
    }

    // Update the file pointer
    (*file_pointer) += data_size;

    // Save the updated inode
    if (read) {
        return 0;
    } else {
        return write_inode(superblock, inode);
    }
}

// Read from inode
int read_from_inode(superblock_t* superblock, inode_t* inode, uint32_t* file_pointer, uint8_t* bytes, int data_size) {
    return _rw_inode(superblock, inode, file_pointer, bytes, data_size, 1);
}

// Write to inode
int write_to_inode(superblock_t* superblock, inode_t* inode, uint32_t* file_pointer, uint8_t* bytes, int data_size) {
    return _rw_inode(superblock, inode, file_pointer, bytes, data_size, 0);
}

/******************************************************
* Directory operations
******************************************************/

// Write a directory to its inode
int write_dir(superblock_t* superblock, directory_t* dir) {
    inode_t inode;

    int id = dir->links[0].inode_id; // ID of "." entry in dir
    read_inode(superblock, id, &inode);

    uint32_t file_pointer = 0;
    if (write_to_inode(superblock, &inode, &file_pointer, (uint8_t*)dir, sizeof(directory_t)) < 0) {
        return -1;
    }
    return id;
}

// Read a directory to its inode
int read_dir(superblock_t* superblock, inode_t* inode, directory_t* dir) {
    uint32_t file_pointer = 0;
    return read_from_inode(superblock, inode, &file_pointer, (uint8_t*)dir, sizeof(directory_t));
}

// Create a directory
int create_directory(superblock_t* superblock, directory_t* parent_dir, char* filename, directory_t* dir) {
    int inode_id = create_file(superblock, parent_dir, filename, INODE_DIRECTORY);
    if (inode_id < 0) {
        return -1;
    }

    file_link_t current_dir_link;
    current_dir_link.inode_id = inode_id;
    memset(current_dir_link.filename, '\0', MAX_FILE_NAME_LENGTH);
    strcpy(current_dir_link.filename, ".");

    file_link_t parent_dir_link;
    parent_dir_link.inode_id = parent_dir->links[0].inode_id; // ID of "." entry in dir
    memset(parent_dir_link.filename, '\0', MAX_FILE_NAME_LENGTH);
    strcpy(parent_dir_link.filename, "..");

    dir->links[0] = current_dir_link;
    dir->links[1] = parent_dir_link;
    dir->files_count = 2;

    return write_dir(superblock, dir);
}

// // Delete a directory, only if empty
// int delete_dir(superblock_t* superblock, directory_t* parent_dir, char* filename) {
//     // Retrive the directory
//     directory_t dir;
//     filename_to_dir(superblock, parent_dir, filename, &dir);
//
//     // If dir not empty, error
//     // NOTE can have "." and ".."
//     if (!(dir.files_count <= 2)) {
//         return -1;
//     }
//
//     inode_t inode;
//     dir_to_inode(superblock, parent_dir, &inode);
//
//     return delete_file(superblock, inode, filename);
// }

// Deletes a file link from a directory
// int delete_file_link(directory_t* dir, char* filename) {
//     // Search directory
//     int index = -1;
//     for (int i=0; i<dir->files_count; i++) {
//         if (strcmp(filename, dir->links[i].filename) == 0) { // Found match
//             index = i;
//             break;
//         }
//     }
//
//     // File not in directory
//     if (index == -1) {
//         return -1;
//     }
//
//     // Update number of files in the directory
//     dir->files_count--;
//
//     // Shift array left one, therefore deleting the file
//     memmove(&dir->links[index+1], &dir->links[index], (dir->files_count-index) * sizeof(file_link_t));
//
//     return 0;
// }

// Returns inode id of filename if exists in the directory
int directory_lookup(directory_t* dir, char* filename) {
    // Search files
    for (int i=0; i<dir->files_count; i++) {
        if (strcmp(filename, dir->links[i].filename) == 0) { // Found match
            return dir->links[i].inode_id;
        }
    }

    return -1;
}

/******************************************************
* File Descriptor Operations
******************************************************/

int add_fd(superblock_t* superblock, file_descriptor_table_t* fdtable, int inode_id, int flags) {
    // Check we can open more files
    if (fdtable->count >= MAX_OPEN_FILES) {
        return -1;
    }

    // Check the file is not already open
    for (int i=0; i < fdtable->count; i++) {
        if (fdtable->open[i].inode_id == inode_id) {
            return -1;
        }
    }

    // Create a new file descriptor
    file_descriptor_t file_descriptor;
    file_descriptor.id = fdtable->count;
    file_descriptor.flags = flags;
    file_descriptor.inode_id = inode_id;

    // Add to table and update the count
    fdtable->open[file_descriptor.id] = file_descriptor;
    fdtable->count++;

    return file_descriptor.id;
}

/******************************************************
* File operations
******************************************************/

// // Delete a file
// int delete_file(superblock_t* superblock, directory_t* dir, char* filename) {
//     inode_t dir_inode;
//     dir_to_inode(superblock, dir, &dir_inode);
//
//     // Read the inode, check for errors
//     inode_t inode;
//     if (filename_to_inode(superblock, &dir_inode, filename, &inode) <0) {
//         return -1;
//     }
//
//     // Delete file link and check status
//     if (delete_file_link(dir, filename) < 0) {
//         return -1;
//     }
//
//     // Write the directory
//     if (write_dir(superblock, &dir_inode, dir) < 0) {
//         return -1;
//     }
//
//     // Finally delete
//     return delete_inode(superblock, &inode);
// }

// Create a new file, returns inode id
int create_file(superblock_t* superblock, directory_t* dir, char* filename, int type) {
    // Check we are not already using that filename
    if (directory_lookup(dir, filename) >= 0) {
        return -1;
    }

    // See if adding another file would be more then the allowed limit
    if (dir->files_count+1 >= MAX_FILES_IN_DIRECTORY) {
        return -1;
    }

    // Create a new inode
    int inode_id = create_inode_type(superblock, type);
    if (inode_id < 0) {
        return -1;
    }

    // Create a new link
    file_link_t file_link;
    file_link.inode_id = inode_id;
    memset(file_link.filename, '\0', MAX_FILE_NAME_LENGTH);
    strcpy(file_link.filename, filename);

    // Update the directory
    dir->links[dir->files_count] = file_link;
    dir->files_count++;

    if (write_dir(superblock, dir) < 0) {
        return -1;
    }

    return inode_id;
}

// Returns file descriptor id
int open_file(superblock_t* superblock, file_descriptor_table_t* fdtable, directory_t* dir, char* path, int flags) {
    int inode_id = path_to_inode_id(superblock, dir, path);
    return add_fd(superblock, fdtable, inode_id, flags);
}

// Close a file, 0 on success
int close_file(superblock_t* superblock, file_descriptor_table_t* fdtable, int file_descriptor_id) {
    // Check the file is not already open
    int index = -1;
    for (int i=0; i < fdtable->count; i++) {
        if (fdtable->open[i].id == file_descriptor_id) {
            index = i;
            break;
        }
    }

    // File not open
    if (index == -1) {
        return -1;
    }

    // Update number of open files
    fdtable->count--;

    // Shift array left one, therefore deleting the file descriptor
    memmove(&fdtable->open[index+1], &fdtable->open[index], (fdtable->count-index) * sizeof(file_descriptor_t));

    return 0;
}

/******************************************************
* Disk Initialisation
******************************************************/

// Create and write empty inodes, returns 0 for success, -1 for failure
int init_inodes(superblock_t* superblock) {
    for (int i=0; i<superblock->inode_num; i++) {
        inode_t inode;
        new_inode(i, &inode);
        if (write_inode(superblock, &inode) < 0) {
            return -1;
        }
    }
    return 0;
}

// Initiate the disk for the first time
int init_disk(superblock_t* superblock, directory_t* root_dir) {
    int status = 0;
    new_superblock(superblock);
    status |= write_superblock(superblock);
    status |= init_inodes(superblock);
    status |= create_root_directory(superblock, root_dir);
    return status;
}

// Create root directory, "." and ".." point to the same location
int create_root_directory(superblock_t* superblock, directory_t* root_dir) {
    int inode_id = create_inode_type(superblock, INODE_DIRECTORY);
    if (inode_id < 0) {
        return -1;
    }

    file_link_t current_dir;
    current_dir.inode_id = inode_id;
    memset(current_dir.filename, '\0', MAX_FILE_NAME_LENGTH);
    strcpy(current_dir.filename, ".");

    file_link_t parent_dir;
    parent_dir.inode_id = inode_id; // NOTE points to self
    memset(parent_dir.filename, '\0', MAX_FILE_NAME_LENGTH);
    strcpy(parent_dir.filename, "..");

    root_dir->links[0] = current_dir;
    root_dir->links[1] = parent_dir;
    root_dir->files_count = 2;

    return write_dir(superblock, root_dir);
}

// Read the root dir
int read_root_dir(superblock_t* superblock, directory_t* dir) {
    inode_t inode;
    if (read_inode(superblock, 0, &inode) < 0) { // NOTE Inode ID 0 is root dir
        return -1;
    }
    return read_dir(superblock, &inode, dir);
}


// IO devices created on disk
// NOTE this is not how it happens in reality however we don't have a file system in memory
int create_io_devices(superblock_t* superblock, file_descriptor_table_t* fdtable, directory_t* root_dir) {
    // Create dev dir
    directory_t dev_dir;
    memset(&dev_dir, 0, sizeof(directory_t));
    if (create_directory(superblock, root_dir, "dev", &dev_dir) < 0) {
        return -1;
    }

    // create each file
    // TODO debug from here, returning -1 for some reason
    if ((create_file(superblock, &dev_dir, "stdin", INODE_DEVICE) < 0)
    || (create_file(superblock, &dev_dir, "stdout", INODE_DEVICE) < 0)
    || (create_file(superblock, &dev_dir, "stderr", INODE_DEVICE) < 0)) {
        return -1;
    }

    return 0;
}
