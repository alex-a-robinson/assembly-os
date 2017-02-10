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

// TODO add pending signals
// TODO add child exit code
// TODO process groups? pgid
typedef struct {
  pid_t pid;
  pid_t ppid;
  ctx_t ctx;
  priority_t priority;
  shared_t shared;
  waiting_t waiting;
} pcb_t;

// Max number of processes
#define MAX_PROCESSES 3

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

#endif
