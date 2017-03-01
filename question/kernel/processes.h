#ifndef __PROCESSES_H
#define __PROCESSES_H

/* The kernel source code is made simpler via three type definitions:
*
* - a type that captures a Process IDentifier (PID), which is really
*   just an integer,
* - a type that captures each component of an execution context (i.e.,
*   processor state) in a compatible order wrt. the low-level handler
*   preservation and restoration prologue and epilogue, and
* - a type that captures a process PCB.
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fs.h"

// Max number of processes
#define MAX_PROCESSES 20
#define PROGRAM_STACK_SIZE 4096
#define MAX_WAITING 20 // Allow waiting for 20 processes
#define MAX_WAITERS 5 // Processes which are watching this process

typedef int pid_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef struct {
  int priority;
  int io_burst;
  int cpu_burst;
  int arrival_time;
  int time_left;
} priority_t;

typedef struct {
  void* ptr;
  int locked;
} shared_t;

typedef struct {
  pid_t pid;
  int result;
} waiting_t;

typedef struct {
  pid_t pid;
} waiter_t;

typedef struct {
  int count;
  int open[MAX_OPEN_FILES];
} fd_proc_table_t;

// TODO add pending signals
// TODO add child exit code
// TODO process groups? pgid
typedef struct {
  pid_t pid;
  pid_t ppid;
  ctx_t ctx;
  priority_t priority;
  shared_t shared;
  waiting_t waiting[MAX_WAITING];
  waiter_t waiters[MAX_WAITERS];
  fd_proc_table_t fd_table;
} pcb_t;

pcb_t* process(pid_t pid);
pcb_t* new_process(pid_t pid, pid_t ppid);
int active_process(pid_t pid);
void init_pcbs();
pid_t free_pid();

void load_ctx(ctx_t* ctx);
void save_ctx(ctx_t* ctx);
void reset_ctx(ctx_t* ctx, pid_t pid);
void set_current(ctx_t* ctx, pid_t pid);

void reset_priority(pcb_t* p);

int share(pid_t pid, void* ptr);
int unshare(pid_t pid, void* ptr);
int lock(pid_t pid, void* ptr);
int unlock(pid_t pid, void* ptr);

waiting_t* get_waiting(pid_t pid, pid_t waiting_pid);
void update_waiters(pid_t pid, int result);
waiting_t* set_waiting(pid_t pid, pid_t waiting_pid);

int add_process_fd(pid_t pid, int fd);
int remove_process_fd(pid_t pid, int fd);
int proc_fd_open(pid_t pid, int file_descriptor_id);

#endif
