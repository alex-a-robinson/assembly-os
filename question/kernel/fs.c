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
void init_disk(superblock_t* superblock, directory_t* root_dir) {
    int status = 0;
    new_superblock(superblock);
    status |= write_superblock(superblock);
    status |= init_inodes(superblock);
    status |= create_root_directory(superblock, root_dir);
    return status;
}

// Create and write empty inodes, returns 0 for success, -1 for failure
int init_inodes(superblock_t* superblock) {
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

uint32_t id_to_addr(uperblock_t* superblock, int id) {
    return superblock->inode_start + id * blocks_occupied(superblock->disk_block_length, sizeof(inode_t));
}

// Read inode by inode id
int read_inode(superblock_t* superblock, int id, inode_t* inode) {
    uint32_t addr = id_to_addr(superblock, id);
    return disk_rd(addr, inode, sizeof(inode_t))
}

// NOTE This will fail if sizeof(inode_t) > block_size, same with superblock
// NOTE Currently giving one block to each inode
// Write inode
int write_inode(superblock_t* superblock, inode_t* inode) {
    uint32_t addr = id_to_addr(superblock, inode->id);
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

// Write a directory to its inode
int write_dir(superblock_t* superblock, inode_t* inode, directory_t* dir) {
    uint32_t file_pointer = 0;
    return write_to_inode(superblock, inode, &file_pointer, dir, sizeof(directory_t));
}

// Read the root dir
int read_root_dir(superblock_t* superblock, directory_t* dir) {
    inode_t* inode;
    if (read_inode(superblock, 0, inode) < 0) { // NOTE Inode ID 0 is root dir
        return -1;
    }
    return read_dir(superblock, inode, dir);
}

// Read a directory to its inode
int read_dir(superblock_t* superblock, inode_t* inode, directory_t* dir) {
    uint32_t file_pointer = 0;
    return read_from_inode(superblock, inode, &file_pointer, dir, sizeof(directory_t));
}

// Create a directory
int create_directory(superblock_t* superblock, inode_t* parent_dir_inode) {
    inode_t* dir_inode;
    if (create_inode_type(superblock, dir_inode, INODE_DIRECTORY) < 0) {
        return -1;
    }

    file_link_t* current_dir;
    file_link->inode_id = dir_inode->id;
    file_link->filename = ".";

    file_link_t* parent_dir;
    file_link->inode_id = parent_dir_inode->id;
    file_link->filename = "..";

    directory_t* dir;
    dir->links[0] = current_dir;
    dir->links[1] = parent_dir;
    dir->file_count = 2;

    uint32_t file_pointer = 0;
    return write_dir(superblock, dir_inode, dir);
}

// Create root directory, "." and ".." point to the same location
int create_root_directory(superblock_t* superblock, directory_t* root_dir) {
    inode_t* dir_inode;
    if (create_inode_type(superblock, dir_inode, INODE_DIRECTORY) < 0) {
        return -1;
    }

    file_link_t* current_dir;
    file_link->inode_id = dir_inode->id;
    file_link->filename = ".";

    file_link_t* parent_dir;
    file_link->inode_id = dir_inode->id;
    file_link->filename = "..";

    directory_t* root_dir;
    root_dir->links[0] = current_dir;
    root_dir->links[1] = parent_dir;
    root_dir->file_count = 2;

    uint32_t file_pointer = 0;
    return write_dir(superblock, root_dir, dir);
}

int dir_to_inode(superblock_t* superblock, directory_t* dir, inode_t* inode) {
    int id = dir->links[0]->inode_id; // ID of "." entry in dir
    return read_inode(superblock, id, inode);

}

// Deletes a file link from a directory
int delete_file_link(directory_t* dir, char* filename) {
    // Search directory
    int index = -1;
    for (int i=0; i<dir->file_count; i++) {
        file_link_t* file_link = dir->links[i];
        if (strcmp(filename, file_link->filename) == 0) { // Found match
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

// Delete a directory, only if empty
int delete_dir(superblock_t* superblock, inode_t* parent_dir_inode, char* filename) {
    // Retrive the directory
    directory_t* dir;
    filename_to_dir(superblock, iparent_dir_inode, filename, dir)

    // If dir not empty, error
    // NOTE can have "." and ".."
    if (!dir->files_count <= 2) {
        return -1;
    }

    return delete_file(superblock, parent_dir_inode, filename);
}

// Delete a file
int delete_file(superblock_t* superblock, inode_t* dir_inode, char* filename) {
    // Retrive the directory
    directory_t* dir;
    if (read_dir(superblock, dir_inode, dir,) < 0) {
        return -1;
    }

    // Read the inode, check for errors
    inode_t* inode;
    if (filename_to_inode(superblock, dir_inode, filename, inode) <0) {
        return -1;
    }

    // Delete file link and check status
    if (delete_file_link(dir, filename) < 0) {
        return -1;
    }

    // Write the directory
    if (write_dir(superblock, dir_inode, dir) < 0) {
        return -1;
    }

    // Finally delete
    return delete_inode(superblock, inode);
}

// Create a new file
int create_file(superblock_t* superblock, inode_t* dir_inode, char* filename) {
    // Create a new inode
    inode_t* inode;
    int status = create_inode_type(superblock, inode, INODE_FILE);

    // Retrive the directory
    directory_t* dir;
    if (read_dir(superblock, dir_inode, dir) < 0) {
        return -1;
    }

    // Check we are not already using that filename
    if (directory_lookup(superblock, dir, filename) < 0) {
        return -1;
    }

    // Create a new link
    file_link_t* file_link;
    file_link->inode_id = inode->id;
    file_link->filename = filename;

    // Update the directory
    dir->links[dir->files_count] = file_link;
    dir->files_count++;
    return write_dir(superblock, dir_inode, dir);
}

// Returns inode id of filename if exists in the directory
int directory_lookup(superblock_t* superblock, inode_t* dir_inode, char* filename) {
    // Read the directory
    uint32_t file_pointer = 0;
    directory_t* dir;
    if (read_from_inode(superblock, dir_inode, &file_pointer, dir, sizeof(directory_t)) < 0) {
        return -1;
    }

    // Search files
    for (int i=0; i<dir->file_count; i++) {
        file_link_t* file_link = dir->links[i];
        if (strcmp(filename, file_link->filename) == 0) { // Found match
            return file_link->inode_id;
        }
    }

    return -1;
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
    inode_t* dir_inode;
    if (filename_to_inode(superblock, parent_dir_inode, filename, dir_inode) < 0) {
        return -1;
    }

    uint32_t file_pointer = 0;
    if (read_from_inode(superblock, dir_inode, &file_pointer, dir, sizeof(directory_t)) < 0) {
        return -1;
    }

    return 0;
}

// Read/Write <size> bytes from <bytes> from file_pointer, returns 0 on success
int _ro_inode(superblock_t* superblock, inode_t* inode, uint32_t* file_pointer, uint8_t* bytes, int data_size, int read) {
    int bytes_left = data_size;
    int block_id = (*file_pointer) / superblock->disk_block_length;
    int offset   = (*file_pointer) % superblock->disk_block_length;

    // Offset only the first address
    unint32_t addr = inode->blocks[i] + offset;
    int bytes_in_block = superblock->disk_block_length - offset;

    // For each avalible block
    for (int i=block_id+1; i<MAX_INODE_BLOCKS; i++) {
        int bytes_to_read_or_writen;
        if (bytes_left < bytes_in_block) { // If fits all in one block
            bytes_to_read_or_writen = bytes_left;
        } else { // Else read/write the entire block
            bytes_to_read_or_writen = bytes_in_block;
        }

        int status;
        if (read) {
            status = disk_rd(addr, bytes, bytes_to_read_or_writen);
        } else {
            status = disk_wr(addr, bytes, bytes_to_read_or_writen);
        }

        // Check for failure
        if (status < 0) {
            return -1;
        }

        // Update how many bytes left & Move pointer
        bytes_left -= bytes_to_read_or_writen;
        bytes += bytes_to_read_or_writen;

        // Check there is still data to read/write
        if (bytes_left <= 0) {
            break;
        }

        // Update for the next block to read/write to
        addr = inode->blocks[i];
        bytes_in_block = superblock->disk_block_length;
    }

    // Check all the data was read/written
    if (bytes_to_read_or_writen > 0) {
        return -1;
    }

    // Update the file pointer
    (*file_pointer) += data_size;

    return 0;
}

// Read from inode
int read_from_inode(superblock_t* superblock, inode_t* inode, uint32_t* file_pointer, uint8_t* bytes, int data_size) {
    return _ro_inode(superblock, inode, file_pointer, bytes, data_size, 1);
}

// Write to inode
int write_to_inode(superblock_t* superblock, inode_t* inode, uint32_t* file_pointer, uint8_t* bytes, int data_size) {
    return _ro_inode(superblock, inode, file_pointer, bytes, data_size, 0);
}

////////////////////////////
// File Descriptors
///////////////////////////

// Finds an inode from a path, return 0 on success
int path_to_inode(superblock_t* superblock, directory_t* dir, inode_t* inode, char* path) {
    inode_t* current_dir_inode;
    inode_t* temp_inode;

    // Load inode of current directory
    if (dir_to_inode(superblock, dir, current_dir_inode) < 0) {
        return -1;
    }

    // Split string on "/"
    char* part;
    part = strtok(path, "/");

    // Until path is empty
    while(part != NULL) {
        if (filename_to_inode(superblock, current_dir_inode, part, temp_inode) < 0) {
            return -1;
        }

        part = strtok(NULL, "/");

        // If we get another directory, continue otherwise if its a file stop
        if (temp_inode->type == INODE_DIRECTORY) {
            current_dir = temp_inode;
        } else if (temp_inode->type == INODE_FILE) {
            if (part == NULL) {
                inode = temp_inode;
                return 0;
            } else { // Found file however still path left, therefore error
                return -1;
            }
        }
    }

    return -1;
}

// Returns file descriptor id
int open_file(superblock_t* superblock, file_descriptor_table_t* fdtable, directory_t* dir, char* path, int flags) {
    // Check we can open more files
    if (fdtable->count >= MAX_OPEN_FILES) {
        return -1;
    }

    // Load the file
    inode_t* inode;
    path_to_inode(superblock, dir, inode, path);

    // Check the file is not already open
    for (int i=0; i < fdtable->count; i++) {
        if (fdtable->open[i]->inode_id == inode->id) {
            return -1;
        }
    }

    // Create a new file descriptor
    file_descriptor_t file_descriptor;
    file_descriptor->id = fdtable->count;
    file_descriptor->flags = flags;
    file_descriptor->inode_id = inode->id;

    // Add to table and update the count
    fdtable->open[file_descriptor->id] = file_descriptor;
    fdtable->count++;

    return file_descriptor->id;
}

// Close a file, 0 on success
int close_file(superblock_t* superblock, file_descriptor_table_t* fdtable, int file_descriptor_id) {
    // Check the file is not already open
    int index = -1;
    for (int i=0; i < fdtable->count; i++) {
        if (fdtable->open[i]->id == file_descriptor_id) {
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

// Returns inode from a file descriptor id, returns 0 on success
int fdid_to_inode(superblock_t* superblock, file_descriptor_table_t* fdtable, int file_descriptor_id, inode_t* inode) {
    for (int i=0; i < fdtable->count; i++) {
        if (fdtable->open[i]->id == file_descriptor_id) {
            return read_inode(superblock, fdtable->open[i]->inode_id, inode);
        }
    }
    return -1;
}
