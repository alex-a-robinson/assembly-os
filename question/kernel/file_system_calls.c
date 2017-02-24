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
    char full_path[MAX_PATH_LENGTH];
    get_full_path(path, full_path);

    // Open the file
    int fd = open_file(mounted, open_files, root_dir, full_path, flags);

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

int process_has_permission(int pid, int fdid) {
    file_descriptor_t* fd = fdid_to_fd(open_files, fdid);

    // FDID not in file descriptors table i.e. not open
    if (fd == NULL) {
        return -1;
    }

    // Check if its open globaly
    if (fd->flags == READ_GLOBAL || fd->flags == WRITE_GLOBAL) {
        return 1;
    }

    // Otherwise check if the process has it open
    return proc_fd_open(pd, fdid);
}

int sys_write(int fd, char* x, int n) {
    // https://linux.die.net/man/2/kill, http://unix.stackexchange.com/questions/80044/how-signals-work-internally

    current->priority.io_burst++;



    // Convert file handler to QEMU devices
    PL011_t* device = UART1; // defult to error
    if (fd == STDOUT_FILENO) {
        device = UART0;
    } else if (fd == STDERR_FILENO) {
        device = UART1;
    }

    for (int i = 0; i < n; i++) {
        PL011_putc(device, *x++, true);
    }
    return n;
}

int sys_read(int fd, char* x, int n) { // NOTE BLOCKING
    // TODO use differnt file handlers

    current->priority.io_burst++;

    PL011_t* device = UART1; // defult to error

    // TODO test this works
    for (int i = 0; i < n; i++) {
        x[i] = PL011_getc(device, true);

        if (x[i] == '\x0A') {
            x[i] = '\x00';
            break;
        }
    }
    return n;
}
