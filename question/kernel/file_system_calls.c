#include "file_system_calls.h"

// NOTE simplification, this would be a mount table
superblock_t* mounted = NULL;
directory_t* root_dir = NULL;
file_descriptor_table_t* open_files;

// Mount a disk, NOTE hard coded device. Returns 0 on success
int sys_mount() {
    if (mounted != NULL) {
        error("Disk already mounted\n");
        return 1;
    }

    int status = read_superblock(mounted);
    if (!valid_superblock(mounted)) {
        status |= init_disk(mounted, root_dir);
    } else {
        status |= read_root_dir(mounted, root_dir);
    }

    if (status < 0) {
        error("Error mounting disk\n");
        return 1;
    }
    return 0;
}

// Unmount a disk, NOTE hard coded device. Returns 0 on success
// NOTE Should we save super block on every opertion?
int sys_unmount() {
    if (mounted == NULL) {
        error("Disk not mounted\n");
        return 1;
    }

    if (write_superblock(mounted) < 0) {
        error("Failed to unmount disk\n");
        return 1;
    }
    mounted = NULL;
    root_dir = NULL;
    return 0;
}

int sys_open(ctx_t* ctx, char* filename) {
    // TODO check file not already open
    // find inode from filename
    // put inode in open_files table + of process

}
