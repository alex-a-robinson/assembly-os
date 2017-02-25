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
    for (uint32_t addr=superblock->data_block_start; addr < superblock->disk_block_num; addr++) {
        if (get_bit(superblock->free_block_bitmap, addr) == 0) {
            return addr;
        }
    }
    return 0;
}

int _rw_sequential_blocks(int block_size, uint32_t first_block_addr, uint8_t* _data, int bytes, int read) {
    uint8_t* data = _data; // Copy so we don't change the pointer TODO check

    int blocks = blocks_occupied(block_size, bytes);
    int bytes_left = bytes;

    int bytes_to_read_or_write;
    for (int addr=first_block_addr; addr<first_block_addr+blocks; addr++) {
        if (bytes_left < block_size) { // If less then an entire block
            bytes_to_read_or_write = bytes_left;
        } else { // Else read/write the entire block
            bytes_to_read_or_write = block_size;
        }

        int status;
        if (read) {
            status = disk_read(block_size, addr, data, bytes_to_read_or_write);
        } else {
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

int read_sequential_blocks(int block_size, uint32_t first_block_addr, uint8_t* data, int bytes) {
    return _rw_sequential_blocks(block_size, first_block_addr, data, bytes, 1);
}

int write_sequential_blocks(int block_size, uint32_t first_block_addr, uint8_t* data, int bytes) {
    return _rw_sequential_blocks(block_size, first_block_addr, data, bytes, 0);
}

/******************************************************
 * Super Block operations
 ******************************************************/

// Read superblock from disk
int read_superblock(superblock_t* superblock) {
    int block_len = disk_get_block_len();
    return read_sequential_blocks(block_len, 0, (uint8_t*)superblock, sizeof(superblock_t));
}

// Returns 1 if superblock is valid else 0
int valid_superblock(superblock_t* superblock) {
    int block_num = disk_get_block_num();
    return (superblock->disk_block_num == block_num);
}

// Write a superblock to disk
int write_superblock(superblock_t* superblock) {
    return write_sequential_blocks(superblock->disk_block_length, 0, (uint8_t*)superblock, sizeof(superblock_t));
}

// Create a new superblock
void new_superblock(superblock_t* superblock) {
    memset(superblock, 0, sizeof(superblock_t));

    int block_num = disk_get_block_num();
    int block_len = disk_get_block_len();
    int inode_num = (int) block_num / MAX_INODE_BLOCKS;

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

int dir_to_inode(superblock_t* superblock, directory_t* dir, inode_t* inode) {
     int id = dir->links[0].inode_id; // ID of "." entry in dir
     return read_inode(superblock, id, inode);
 }

 // Return a inode from a filename
int filename_to_inode(superblock_t* superblock, inode_t* dir_inode, char* filename, inode_t* inode) {
     int inode_id = directory_lookup(superblock, dir_inode, filename);

     // Check we found the inode
     if (inode_id < 0) {
         return -1;
     }

     // Read the inode, check for errors
     if (read_inode(superblock, inode_id, inode) < 0) {
         return -1;
     }

     return 0;
 }

 // Return a directory from a filename
int filename_to_dir(superblock_t* superblock, inode_t* parent_dir_inode, char* filename, directory_t* dir) {
     inode_t dir_inode;
     if (filename_to_inode(superblock, parent_dir_inode, filename, &dir_inode) < 0) {
         return -1;
     }

     uint32_t file_pointer = 0;
     if (read_from_inode(superblock, &dir_inode, &file_pointer, (uint8_t*)dir, sizeof(directory_t)) < 0) {
         return -1;
     }

     return 0;
 }

 // Finds an inode from a path, return 0 on success
int path_to_inode(superblock_t* superblock, directory_t* dir, inode_t* inode, char* path) {
     inode_t current_dir_inode;
     inode_t temp_inode;

     // Load inode of current directory
     if (dir_to_inode(superblock, dir, &current_dir_inode) < 0) {
         return -1;
     }

     // Split string on "/"
     char* part;
     part = strtok(path, "/");

     // Until path is empty
     while(part != NULL) {
         if (filename_to_inode(superblock, &current_dir_inode, part, &temp_inode) < 0) {
             return -1;
         }

         part = strtok(NULL, "/");

         // If we get another directory, continue otherwise if its a file stop
         if (temp_inode.type == INODE_DIRECTORY) {
             current_dir_inode = temp_inode;
             continue;
         } else if (temp_inode.type == INODE_FILE) {
             if (part == NULL) {
                 inode = &temp_inode;
                 return 0;
             } else { // Found file however still path left, therefore error
                 return -1;
             }
         } else { // unknown file type
             return -1;
         }
     }

     return -1;
 }

// Returns inode from a file descriptor id, returns 0 on success
int fdid_to_inode(superblock_t* superblock, file_descriptor_table_t* fdtable, int file_descriptor_id, inode_t* inode) {
    file_descriptor_t* fd = fdid_to_fd(fdtable, file_descriptor_id); // TODO not sure if this is correct

    // Failed to get file descriptor
    if (fd == NULL) {
        return -1;
    }

    return read_inode(superblock, fd->inode_id, inode);
}

// Returns file descriptor from a file descriptor id
file_descriptor_t* fdid_to_fd(file_descriptor_table_t* fdtable, int file_descriptor_id) {
    for (int i=0; i < fdtable->count; i++) {
        if (fdtable->open[i].id == file_descriptor_id) {
            return &fdtable->open[i];
        }
    }
    return NULL;
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
    return read_sequential_blocks(superblock->disk_block_length, addr, (uint8_t*)inode, sizeof(inode_t));
}

// NOTE This will fail if sizeof(inode_t) > block_size, same with superblock
// NOTE Currently giving one block to each inode
// Write inode
int write_inode(superblock_t* superblock, inode_t* inode) {
    uint32_t addr = id_to_addr(superblock, inode->id);
    return write_sequential_blocks(superblock->disk_block_length, addr, (uint8_t*)inode, sizeof(inode_t));
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

int create_inode_type(superblock_t* superblock, inode_t* inode, int inode_type) {
    int status = free_inode(superblock, inode);

    // Mark allocated in superblock
    set_bit(superblock->free_inode_bitmap, inode->id);

    inode->type = inode_type;
    inode->creation_time = 1; // TODO
    status |= write_inode(superblock, inode);

    return status;
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

    int bytes_left = data_size;
    int block_id = (*file_pointer) / superblock->disk_block_length;
    int offset   = (*file_pointer) % superblock->disk_block_length;

    // Offset only the first address
    uint32_t addr = inode->blocks[block_id] + offset;
    int bytes_in_block = superblock->disk_block_length - offset;

    int bytes_to_read_or_write;

    // For each avalible block
    for (int i=block_id+1; i<MAX_INODE_BLOCKS; i++) {
        if (bytes_left < bytes_in_block) { // If fits all in one block
            bytes_to_read_or_write = bytes_left;
        } else { // Else read/write the entire block
            bytes_to_read_or_write = bytes_in_block;
        }

        int status;
        if (read) {
            status = disk_read(superblock->disk_block_length, addr, bytes, bytes_to_read_or_write);
        } else {
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

        // Update for the next block to read/write to
        addr = inode->blocks[i];
        bytes_in_block = superblock->disk_block_length;
    }

    // Update the file pointer
    (*file_pointer) += data_size;

    return 0;
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
int write_dir(superblock_t* superblock, inode_t* inode, directory_t* dir) {
    uint32_t file_pointer = 0;
    return write_to_inode(superblock, inode, &file_pointer, (uint8_t*)dir, sizeof(directory_t));
}

// Read a directory to its inode
int read_dir(superblock_t* superblock, inode_t* inode, directory_t* dir) {
    uint32_t file_pointer = 0;
    return read_from_inode(superblock, inode, &file_pointer, (uint8_t*)dir, sizeof(directory_t));
}

// Create a directory
int create_directory(superblock_t* superblock, inode_t* parent_dir_inode, inode_t* dir_inode, char* filename) {
    if (create_file(superblock, parent_dir_inode, filename, INODE_DIRECTORY, dir_inode) < 0) {
        return -1;
    }

    file_link_t current_dir;
    current_dir.inode_id = dir_inode->id;
    strcpy(current_dir.filename, ".");

    file_link_t parent_dir;
    parent_dir.inode_id = parent_dir_inode->id;
    strcpy(parent_dir.filename, "..");

    directory_t dir;
    dir.links[0] = current_dir;
    dir.links[1] = parent_dir;
    dir.files_count = 2;

    return write_dir(superblock, dir_inode, &dir);
}

// Delete a directory, only if empty
int delete_dir(superblock_t* superblock, inode_t* parent_dir_inode, char* filename) {
    // Retrive the directory
    directory_t dir;
    filename_to_dir(superblock, parent_dir_inode, filename, &dir);

    // If dir not empty, error
    // NOTE can have "." and ".."
    if (!(dir.files_count <= 2)) {
        return -1;
    }

    return delete_file(superblock, parent_dir_inode, filename);
}

// Deletes a file link from a directory
int delete_file_link(directory_t* dir, char* filename) {
    // Search directory
    int index = -1;
    for (int i=0; i<dir->files_count; i++) {
        if (strcmp(filename, dir->links[i].filename) == 0) { // Found match
            index = i;
            break;
        }
    }

    // File not in directory
    if (index == -1) {
        return -1;
    }

    // Update number of files in the directory
    dir->files_count--;

    // Shift array left one, therefore deleting the file
    memmove(&dir->links[index+1], &dir->links[index], (dir->files_count-index) * sizeof(file_link_t));

    return 0;
}

// Returns inode id of filename if exists in the directory
int directory_lookup(superblock_t* superblock, inode_t* dir_inode, char* filename) {
    // Read the directory
    uint32_t file_pointer = 0;
    directory_t dir;
    if (read_from_inode(superblock, dir_inode, &file_pointer, (uint8_t*)&dir, sizeof(directory_t)) < 0) {
        return -1;
    }

    // Search files
    for (int i=0; i<dir.files_count; i++) {
        if (strcmp(filename, dir.links[i].filename) == 0) { // Found match
            return dir.links[i].inode_id;
        }
    }

    return -1;
}

/******************************************************
 * File Descriptor Operations
 ******************************************************/

int add_fd(superblock_t* superblock, file_descriptor_table_t* fdtable, inode_t* inode, int flags) {
    // Check we can open more files
    if (fdtable->count >= MAX_OPEN_FILES) {
        return -1;
    }

    // Check the file is not already open
    for (int i=0; i < fdtable->count; i++) {
        if (fdtable->open[i].inode_id == inode->id) {
            return -1;
        }
    }

    // Create a new file descriptor
    file_descriptor_t file_descriptor;
    file_descriptor.id = fdtable->count;
    file_descriptor.flags = flags;
    file_descriptor.inode_id = inode->id;

    // Add to table and update the count
    fdtable->open[file_descriptor.id] = file_descriptor;
    fdtable->count++;

    return file_descriptor.id;
}

/******************************************************
 * File operations
 ******************************************************/

 // Delete a file
 int delete_file(superblock_t* superblock, inode_t* dir_inode, char* filename) {
     // Retrive the directory
     directory_t dir;
     if (read_dir(superblock, dir_inode, &dir) < 0) {
         return -1;
     }

     // Read the inode, check for errors
     inode_t inode;
     if (filename_to_inode(superblock, dir_inode, filename, &inode) <0) {
         return -1;
     }

     // Delete file link and check status
     if (delete_file_link(&dir, filename) < 0) {
         return -1;
     }

     // Write the directory
     if (write_dir(superblock, dir_inode, &dir) < 0) {
         return -1;
     }

     // Finally delete
     return delete_inode(superblock, &inode);
 }

 // Create a new file
 int create_file(superblock_t* superblock, inode_t* dir_inode, char* filename, int type, inode_t* inode) {
     // Retrive the directory
     directory_t dir;
     if (read_dir(superblock, dir_inode, &dir) < 0) {
         return -1;
     }

     // Check we are not already using that filename
     if (directory_lookup(superblock, dir_inode, filename) < 0) {
         return -1;
     }

     // Create a new inode
     if (create_inode_type(superblock, inode, type) < 0) {
         return -1;
     }

     // Create a new link
     file_link_t file_link;
     file_link.inode_id = inode->id;
     strcpy(file_link.filename, filename);

     // Update the directory
     dir.links[dir.files_count] = file_link;
     dir.files_count++;

     return write_dir(superblock, dir_inode, &dir);
 }

// Returns file descriptor id
int open_file(superblock_t* superblock, file_descriptor_table_t* fdtable, directory_t* dir, char* path, int flags) {
    // Load the file
    inode_t inode;
    path_to_inode(superblock, dir, &inode, path);

    return add_fd(superblock, fdtable, &inode, flags);
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
    inode_t dir_inode;
    if (create_inode_type(superblock, &dir_inode, INODE_DIRECTORY) < 0) {
        return -1;
    }

    file_link_t current_dir;
    current_dir.inode_id = dir_inode.id;
    strcpy(current_dir.filename, ".");

    file_link_t parent_dir;
    parent_dir.inode_id = dir_inode.id; // NOTE points to self
    strcpy(parent_dir.filename, "..");

    root_dir->links[0] = current_dir;
    root_dir->links[1] = parent_dir;
    root_dir->files_count = 2;

    return write_dir(superblock, &dir_inode, root_dir);
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
    // Load root dir
    inode_t root_dir_i;
    if (dir_to_inode(superblock, root_dir, &root_dir_i) < 0) {
        return -1;
    }

    // Create dev dir
    inode_t dev_dir_inode;
    if (create_directory(superblock, &root_dir_i, &dev_dir_inode, "dev") < 0) {
        return -1;
    }

    // create each file
    inode_t stdin_i;
    inode_t stdout_i;
    inode_t stderr_i;
    if ((create_file(superblock, &dev_dir_inode, "stdin", INODE_DEVICE, &stdin_i) < 0)
        || (create_file(superblock, &dev_dir_inode, "stdout", INODE_DEVICE, &stdout_i) < 0)
        || (create_file(superblock, &dev_dir_inode, "stderr", INODE_DEVICE, &stderr_i) < 0)) {
         return -1;
    }

    return 0;
}
