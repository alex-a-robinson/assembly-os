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

// TODO add pending signals
// TODO add child exit code
// TODO process groups? pgid
typedef struct {
  pid_t pid;
  pid_t ppid;
  ctx_t ctx;
} pcb_t;

// Max number of processes
#define MAX_PROCESSES 3

pcb_t* process(pid_t pid);
int active_process(pid_t pid);
void init_pcbs();
pid_t free_pid();

void load_ctx(ctx_t* ctx);
void save_ctx(ctx_t* ctx);
void reset_ctx(ctx_t* ctx, pid_t pid);
void set_current(ctx_t* ctx, pid_t pid);

#endif
