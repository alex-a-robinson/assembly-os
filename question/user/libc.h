#ifndef __LIBC_H
#define __LIBC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Define a type that that captures a Process IDentifier (PID).

typedef int pid_t;

/* The definitions below capture symbolic constants within these classes:
 *
 * 1. system call identifiers (i.e., the constant used by a system call
 *    to specify which action the kernel should take),
 * 2. signal identifiers (as used by the kill system call),
 * 3. status codes for exit,
 * 4. standard file descriptors (e.g., for read and write system calls),
 * 5. platform-specific constants, which may need calibration (wrt. the
 *    underlying hardware QEMU is executed on).
 *
 * They don't *precisely* match the standard C library, but are intended
 * to act as a limited model of similar concepts.
 */

#define SYS_YIELD     ( 0x00 )
#define SYS_WRITE     ( 0x01 )
#define SYS_READ      ( 0x02 )
#define SYS_FORK      ( 0x03 )
#define SYS_EXIT      ( 0x04 )
#define SYS_EXEC      ( 0x05 )
#define SYS_KILL      ( 0x06 )
#define SYS_PS        ( 0x07 )
#define SYS_SHARE     ( 0x08 )
#define SYS_UNSHARE   ( 0x09 )
#define SYS_LOCK      ( 0x10 )
#define SYS_UNLOCK    ( 0x11 )
#define SYS_WAIT      ( 0x12 )
#define SYS_FORKWAIT  ( 0x13 )
#define SYS_OPEN      ( 0x14 )
#define SYS_CLOSE     ( 0x15 )
#define SYS_MOUNT     ( 0x16 )
#define SYS_UNMOUNT   ( 0x17 )
#define SYS_LS        ( 0x18 )
#define SYS_STAT      ( 0x19 )

#define SIG_TERM      ( 0x00 )
#define SIG_QUIT      ( 0x01 )

#define EXIT_SUCCESS  ( 0 )
#define EXIT_FAILURE  ( 1 )

// TODO We need to get these instead of declaring them

#define  STDIN_FILENO ( 0 )
#define STDOUT_FILENO ( 1 )
#define STDERR_FILENO ( 2 )

#define MAX_PATH_LENGTH 1024
#define MAX_FILE_NAME_LENGTH 100
#define MAX_FILES_IN_DIRECTORY 10

typedef struct {
    int type;
    int size;
    uint32_t creation_time;
    uint32_t modification_time;
} file_stat_t;

// inode types
#define INODE_UNALLOCATED ( 0 )
#define INODE_FILE        ( 1 )
#define INODE_DIRECTORY   ( 2 )
#define INODE_DEVICE      ( 3 )

// File descriptors constants
#define READ 0
#define WRITE 1
#define READ_WRITE 2

// convert ASCII string x into integer r
extern int  atoi( char* x        );
// convert integer x into ASCII string r
extern void itoa( char* r, int x );
char* ss(char* b, int x);

// cooperatively yield control of processor, i.e., invoke the scheduler
extern void yield();

// write n bytes from x to   the file descriptor fd; return bytes written
extern int write( int fd, const void* x, size_t n );
extern int open(const void* path, int flags);
extern int close(int fd);
extern int mount();
extern int unmount();

void err(char* msg);
void puts(char* msg);

// read  n bytes into x from the file descriptor fd; return bytes read
extern int  read( int fd,       void* x, size_t n );

// perform fork, returning 0 iff. child or > 0 iff. parent process
extern int  fork();
// perform exit, i.e., terminate process with status x
void ps(int pid);
extern void exit(       int   x );
// perform exec, i.e., start executing program at address x
void exec( const void* x , char* args);

// signal process identified by pid with signal x
extern int  kill( pid_t pid, int x );

extern int sharem(void* ptr);
extern int unsharem(void* ptr);
extern int lockm(void* ptr);
extern int unlockm(void* ptr);

void sleep(int t);

int waitp(int pid);
int waitpnb(int pid);
int fork_wait();

int ls(char* path, char* file_list);
int stat(char* path, file_stat_t* file_info);

int parse_cmd(char* _input, char* cmd, char* args);
void path_from_args(char* working_dir, char* args, char* path);

#endif
