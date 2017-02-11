#ifndef __SYSTEM_CALLS_H
#define __SYSTEM_CALLS_H

#include "PL011.h"

#include "processes.h"
#include "utilities.h"
#include "scheduler.h"

#include <string.h>


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

#define SIG_TERM      ( 0x00 )
#define SIG_QUIT      ( 0x01 )

#define EXIT_SUCCESS  ( 0 )
#define EXIT_FAILURE  ( 1 )

#define  STDIN_FILENO ( 0 )
#define STDOUT_FILENO ( 1 )
#define STDERR_FILENO ( 2 )

int sys_write(int fd, char* x, int n);
int sys_read(int fd, char* x, int n);
void sys_fork(ctx_t* ctx);
void sys_exit(ctx_t* ctx, int x);
int sys_kill(ctx_t* ctx, pid_t pid, uint32_t sig);
void sys_yield(ctx_t* ctx);
void sys_exec(ctx_t* ctx, void* x);
void fix_orphaned_processes(pid_t ppid);
void ps_stats(pid_t pid);
void sys_ps();

int sys_share(ctx_t* ctx, void* ptr);
int sys_unshare(ctx_t* ctx, void* ptr);
int sys_lock(ctx_t* ctx, void* ptr);
int sys_unlock(ctx_t* ctx, void* ptr);

int sys_wait(ctx_t* ctx, pid_t pid);

#endif
