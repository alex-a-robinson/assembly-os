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
int sys_ls(char* path, char* file_list);
int sys_stat(char* path, file_stat_t* file_info);
int sys_mkdir(char *path_and_filename);
int sys_rmdir(char *path_and_filename);
int sys_rm(char *path_and_filename);

#endif
