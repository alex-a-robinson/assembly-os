#include "file_system_calls.h"

// NOTE simplification, this would be a mount table

superblock_t _mounted;
superblock_t* mounted = &_mounted;

directory_t _root_dir;
directory_t* root_dir = &_root_dir;

file_descriptor_table_t _open_files;
file_descriptor_table_t* open_files = &_open_files;

extern pcb_t* current;

int STDIN_FD = -1;
int STDOUT_FD = -1;
int STDERR_FD = -1;

void init_disk_globals() {
    memset(mounted, 0, sizeof(superblock_t));
    memset(root_dir, 0, sizeof(directory_t));
    memset(open_files, 0, sizeof(file_descriptor_table_t));
}

// Mount a disk, NOTE hard coded device. Returns 0 on success
int sys_mount() {
    init_disk_globals();
    if (valid_superblock(mounted)) {
        error("Disk already mounted\n");
        return -1;
    }

    int status = read_superblock(mounted);

    if (!valid_superblock(mounted)) {
        status |= init_disk(mounted, root_dir);
        status |= create_io_devices(mounted, open_files, root_dir);
    } else {
        status |= read_root_dir(mounted, root_dir);
    }

    status |= open_io_devices();

    write_superblock(mounted);


    if (status < 0) {
        error("Error mounting disk\n");
        return -1;
    }

    return 0;
}

// Open IO devices
int open_io_devices() {
    STDIN_FD  = open_file(mounted, open_files, root_dir, "/dev/stdin", READ_GLOBAL);
    STDOUT_FD = open_file(mounted, open_files, root_dir, "/dev/stdout", WRITE_GLOBAL);
    STDERR_FD = open_file(mounted, open_files, root_dir, "/dev/stderr", WRITE_GLOBAL);

    // Return error if any failed
    if (STDIN_FD < 0 || STDOUT_FD < 0 || STDERR_FD < 0) {
        return -1;
    }

    return 0;
}

// Unmount a disk, NOTE hard coded device. Returns 0 on success
// NOTE Should we save super block on every opertion?
int sys_unmount() {
    if (mounted == NULL) {
        error("Disk not mounted\n");
        return -1;
    }

    // Close all open files
    for (int i=0; i < open_files->count; i++) {
        if (close_file(mounted, open_files, open_files->open[i].id) < 0) {
            error("Couldn't close all open files\n");
            return -1;
        }
    }

    if (write_superblock(mounted) < 0) {
        error("Failed to unmount disk\n");
        return -1;
    }
    mounted = NULL;
    root_dir = NULL;
    return 0;
}

// Open a file, returns -1 on error otherwise the file descriptros id
int sys_open(char* path, int flags) {

    // Open the file
    int fd = open_file(mounted, open_files, root_dir, path, flags);

    // Error failed to open
    if (fd < 0) {
        error("Failed to open file\n");
        return -1;
    }

    // Update root dir as may have changed if a new file has been added
    if (read_root_dir(mounted, root_dir) < 0) {
        error("Failed to read root dir\n");
        return -1;
    }

    // Add from process open files table & check for an error
    if (add_process_fd(current->pid, fd) < 0) {
        return -1;
    }

    return fd;
}

// Close file, -1 on error, 0 on success
int sys_close(int fd) {
    // Open the file
    if (close_file(mounted, open_files, fd) < 0) {
        error("Failed to close file\n");
        return -1;
    }

    // Remove from process open files table
    return remove_process_fd(current->pid, fd);
}

int process_has_file_permission(int pid, int fdid, int mode) {
    file_descriptor_t* fd = fdid_to_fd(open_files, fdid);

    // FDID not in file descriptors table i.e. not open
    if (fd == NULL) {
        return -1;
    }

    // Check if its open globaly
    if ((mode == READ && fd->flags == READ_GLOBAL) ||
    (mode == WRITE && fd->flags == WRITE_GLOBAL)) {
        return 1;
    }

    // File not open in the correct mode e.g. asking for READ but file is WRITE only
    if ((fd->flags != READ_WRITE) && (fd->flags != mode)) {
        return -1;
    }

    // Otherwise check if the process has it open
    return proc_fd_open(pid, fdid);
}

// Write to a device
int write_device(int fd, char* x, int n) {
    // Convert file handler to QEMU devices
    PL011_t* device = UART1; // defult to error
    if (fd == STDOUT_FD) {
        device = UART0;
    } else if (fd == STDERR_FD) {
        device = UART1;
    } else {
        error("Unknown device\n");
        return -1;
    }

    for (int i = 0; i < n; i++) {
        PL011_putc(device, *x++, true);
    }
    return n;
}

// Read from a device
int read_device(int fd, char* x, int n) {
    // Convert file handler to QEMU devices
    PL011_t* device = UART1; // defult to error // TODO using the correct devices?
    if (fd == STDIN_FD) {
        device = UART1;
    } else {
        error("Unknown device\n");
        return -1;
    }

    for (int i = 0; i < n; i++) {
        x[i] = PL011_getc(device, true);

        if (x[i] == '\x1B') { // Escape
            break;
        }
    }
    return n;
}

int sys_write(int fd, char* x, int n) {
    //current->priority.io_burst++;

    if (process_has_file_permission(current->pid, fd, WRITE) < 0) {
        error("File not open\n");
        return -1;
    }

    inode_t inode;
    if (fdid_to_inode(mounted, open_files, fd, &inode) < 0) {
        error("Failed to open\n");
        return -1;
    }

    // If file is a device
    if (inode.type == INODE_DEVICE) {
        return write_device(fd, x, n);
    } else if (inode.type != INODE_FILE) {
        error("Cannot write to this type of file\n");
        return -1;
    }

    uint32_t file_pointer = 0; // TODO not using at the moment
    if (write_to_inode(mounted, &inode, &file_pointer, (uint8_t*)x, n) < 0) {
        error("Failed to write\n");
        return -1;
    }

    return n;
}

int sys_read(int fd, char* x, int n) { // NOTE BLOCKING

    if (process_has_file_permission(current->pid, fd, READ) < 0) {
        error("File not open\n");
        return -1;
    }

    inode_t inode;
    if (fdid_to_inode(mounted, open_files, fd, &inode) < 0) {
        error("Failed to open\n");
        return -1;
    }

    // If file is a device
    if (inode.type == INODE_DEVICE) {
        current->priority.io_burst++;
        return read_device(fd, x, n);
    } else if (inode.type != INODE_FILE) {
        error("Cannot read this type of file\n");
        return -1;
    }

    uint32_t file_pointer = 0; // TODO not using at the moment
    if (read_from_inode(mounted, &inode, &file_pointer, (uint8_t*)x, n) < 0) {
        error("Failed to read\n");
        return -1;
    }

    return n;
}

int sys_ls(char* path, char* file_list) {
    int inode_id = path_to_inode_id(mounted, root_dir, path);

    if (inode_id < 0) {
        return -1;
    }

    inode_t inode;
    if (read_inode(mounted, inode_id, &inode) < 0) {
        return -1;
    }
    directory_t dir;
    if (read_dir(mounted, &inode, &dir) < 0) {
        error("This is not a directory\n");
        return -1;
    }

    for (int i=0; i < dir.files_count; i++) {
        strcat(file_list, dir.links[i].filename);
        strcat(file_list, "\n");
    }

    return 0;
}

int sys_stat(char* path, file_stat_t* file_info) {
    int inode_id = path_to_inode_id(mounted, root_dir, path);

    if (inode_id < 0) {
        error("No such file\n");
        return -1;
    }

    inode_t inode;
    if (read_inode(mounted, inode_id, &inode) < 0) {
        return -1;
    }

    file_info->type = inode.type;
    file_info->size = inode.size;
    file_info->creation_time = inode.creation_time;
    file_info->modification_time = inode.modification_time;

    return 0;
}

// TODO check
int sys_mkdir(char *path_and_filename) {
    char filename[MAX_FILE_NAME_LENGTH];
    char path[MAX_PATH_LENGTH];
    if (parse_filename(path_and_filename, filename, path) < 0) {
        error("Failed to parse path\n");
        return -1;
    }

    directory_t parent_dir;
    if (strlen(path) == 0) {
        memcpy(&parent_dir, root_dir, sizeof(directory_t));
    } else {
        inode_t inode;
        int dir_inode_id = path_to_inode_id(mounted, root_dir, path);
        if (read_inode(mounted, dir_inode_id, &inode) < 0) {
            error("Failed to read inode\n");
            return -1;
        }
        if (read_dir(mounted, &inode, &parent_dir) < 0) {
            error("Failed to read dir\n");
            return -1;
        }
    }

    // Create dir
    directory_t dir;
    memset(&dir, 0, sizeof(directory_t));
    if (create_directory(mounted, &parent_dir, filename, &dir) < 0) {
        error("Failed to write dir\n");
        return -1;
    }

    // Update root dir as may have changed if a new file has been added
    if (read_root_dir(mounted, root_dir) < 0) {
        error("Failed to read root dir\n");
        return -1;
    }
}

// TODO check
int sys_rmdir(char *path_and_filename) {
    char filename[MAX_FILE_NAME_LENGTH];
    char path[MAX_PATH_LENGTH];
    if (parse_filename(path_and_filename, filename, path) < 0) {
        error("Failed to parse path\n");
        return -1;
    }

    inode_t inode;
    directory_t parent_dir;
    if (strlen(path) == 0) {
        memcpy(&parent_dir, root_dir, sizeof(directory_t));
    } else {
        int dir_inode_id = path_to_inode_id(mounted, root_dir, path);
        read_inode(mounted, dir_inode_id, &inode);
        read_dir(mounted, &inode, &parent_dir);
    }

    directory_t dir_to_delete;
    int inode_id = path_to_inode_id(mounted, &parent_dir, filename);
    read_inode(mounted, inode_id, &inode);
    if (read_dir(mounted, &inode, &dir_to_delete) < 0) {
        error("File is not a directory\n");
        return -1;
    }

    if (dir_to_delete.files_count > 2) {
        error("Directory not empty\n");
        return -1;
    }

    delete_file_link(&parent_dir, filename);
    write_dir(mounted, &parent_dir);
    return delete_inode(mounted, &inode);

    if (delete_inode(mounted, &inode) < 0) {
        error("Failed to delete inode\n");
        return -1;
    }

    // Update root dir as may have changed if a new file has been added
    if (read_root_dir(mounted, root_dir) < 0) {
        error("Failed to read root dir\n");
        return -1;
    }

}

// TODO check these work
int sys_rm(char *path_and_filename) {
    char filename[MAX_FILE_NAME_LENGTH];
    char path[MAX_PATH_LENGTH];
    if (parse_filename(path_and_filename, filename, path) < 0) {
        error("Failed to parse path\n");
        return -1;
    }

    inode_t inode;
    directory_t parent_dir;
    if (strlen(path) == 0) {
        memcpy(&parent_dir, root_dir, sizeof(directory_t));
    } else {
        int dir_inode_id = path_to_inode_id(mounted, root_dir, path);
        read_inode(mounted, dir_inode_id, &inode);
        read_dir(mounted, &inode, &parent_dir);
    }

    int inode_id = path_to_inode_id(mounted, &parent_dir, filename);
    if (read_inode(mounted, inode_id, &inode) < 0) {
        return -1;
    }

    if (delete_file_link(&parent_dir, filename) < 0) {
        error("Failed to delete link\n");
        return -1;
    }
    if (write_dir(mounted, &parent_dir) < 0) {
        error("Failed to write dir\n");
        return -1;
    }
    if (delete_inode(mounted, &inode) < 0) {
        error("Failed to delete inode\n");
        return -1;
    }

    // Update root dir as may have changed if a new file has been added
    if (read_root_dir(mounted, root_dir) < 0) {
        error("Failed to read root dir\n");
        return -1;
    }
}
