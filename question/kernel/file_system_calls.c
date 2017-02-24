#include "file_system_calls.h"

// NOTE simplification, this would be a mount table
superblock_t* mounted = NULL;
directory_t* root_dir = NULL;
file_descriptor_table_t* open_files;

extern pcb_t* current;

extern STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO;

// Mount a disk, NOTE hard coded device. Returns 0 on success
int sys_mount() {
    if (mounted != NULL) {
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

    if (status < 0) {
        error("Error mounting disk\n");
        return -1;
    }
    return 0;
}

// Open IO devices
int open_io_devices() {
    STDIN_FILENO  = open_file(mounted, open_files, root_dir, "/dev/stdin", READ_GLOBAL);
    STDOUT_FILENO = open_file(mounted, open_files, root_dir, "/dev/stdout", WRITE_GLOBAL);
    STDERR_FILENO = open_file(mounted, open_files, root_dir, "/dev/stderr", WRITE_GLOBAL);

    // Return error if any failed
    if (STDIN_FILENO < 0 || STDOUT_FILENO < 0 || STDERR_FILENO < 0) {
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
        if (close_file(mounted, open_files, open_files->open[i]) < 0) {
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
int sys_open(ctx_t* ctx, char* path, int flags) {
    //char full_path[MAX_PATH_LENGTH];
    //get_full_path(path, full_path);

    // Open the file
    int fd = open_file(mounted, open_files, root_dir, path, flags);

    // Error failed to open
    if (fd < 0) {
        error("Failed to open file\n");
        return -1;
    }

    // Add from process open files table & check for an error
    if (add_fd(current->pid, fd) < 0) {
        return -1;
    }

    return fd;
}

// Close file, -1 on error, 0 on success
int sys_close(ctx_t* ctx, int fd) {

    // Open the file
    if (close_file(mounted, open_files, fd) < 0) {
        error("Failed to close file\n");
        return -1;
    }

    // Remove from process open files table
    return remove_fd(current->pid, fd);
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
    return proc_fd_open(pd, fdid);
}

// Write to a device
int write_device(int fd, char* x, int n) {
    // Convert file handler to QEMU devices
    PL011_t* device = UART1; // defult to error
    if (fd == STDOUT_FILENO) {
        device = UART0;
    } else if (fd == STDERR_FILENO) {
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
    PL011_t* device = UART1; // defult to error
    if (fd == STDIN_FILENO) {
        device = UART1;
    } else {
        error("Unknown device\n");
        return -1;
    }

    for (int i = 0; i < n; i++) {
        x[i] = PL011_getc(device, true);

        if (x[i] == '\x0A') {
            x[i] = '\x00';
            break;
        }
    }
    return n;
}

int sys_write(int fd, char* x, int n) {
    current->priority.io_burst++;

    if (process_has_file_permission(current->pid, fd, WRITE) < 0) {
        error("File not open\n");
        return -1;
    }

    inode_t* inode;
    if (fdid_to_inode(mounted, open_files, fd, inode) < 0) {
        error("Failed to open\n");
        return -1;
    }

    // If file is a device
    if (inode->type == INODE_DEVICE) {
        return write_device(fd, x, n);
    } else if (inode->type != INODE_FILE) {
        error("Cannot write to this type of file\n");
        return -1;
    }

    uint32_t* file_pointer; // TODO not using at the moment
    if (write_to_inode(mounted, inode, file_pointer, x, n) < 0) {
        error("Failed to write\n");
        return -1;
    }

    return n;
}

int sys_read(int fd, char* x, int n) { // NOTE BLOCKING
    current->priority.io_burst++;

    if (process_has_file_permission(current->pid, fd, READ) < 0) {
        error("File not open\n");
        return -1;
    }

    inode_t* inode;
    if (fdid_to_inode(mounted, open_files, fd, inode) < 0) {
        error("Failed to open\n");
        return -1;
    }

    // If file is a device
    if (inode->type == INODE_DEVICE) {
        return read_device(fd, x, n);
    } else if (inode->type != INODE_FILE) {
        error("Cannot read this type of file\n");
        return -1;
    }

    uint32_t* file_pointer; // TODO not using at the moment
    if (read_from_inode(mounted, inode, file_pointer, x, n) < 0) {
        error("Failed to read\n");
        return -1;
    }

    return n;
}
