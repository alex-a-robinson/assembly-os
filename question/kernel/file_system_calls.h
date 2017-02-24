#ifndef __FILE_SYSTEM_CALLS_H
#define __FILE_SYSTEM_CALLS_H

#include "processes.h"
#include "fs.h"
#include "utilities.h"

int sys_mount();
int open_io_devices();
int sys_unmount();
int sys_open(char* path, int flags);
int sys_close(int fd);
int process_has_file_permission(int pid, int fdid, int mode);
int write_device(int fd, char* x, int n);
int read_device(int fd, char* x, int n);
int sys_write(int fd, char* x, int n);
int sys_read(int fd, char* x, int n);

#endif
