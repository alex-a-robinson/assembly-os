#include "system_calls.h"

extern pcb_t* current;

void sys_fork(ctx_t* ctx) {
    // https://linux.die.net/man/2/fork
    // NOTE what to do about file descriptors?
    pid_t pid = free_pid();

    // If no pid, return an error
    if (pid == -1) {
        error("No free processes\n");
        ctx->gpr[0] = (uint32_t)-1;
        return;
    }

    // Copy the process
    pcb_t* p = new_process(pid, current->pid);
    memcpy(&p->ctx, ctx, sizeof(ctx_t));

    // Return child pid to parent and 0 to child
    ctx->gpr[0] = (uint32_t)pid;
    p->ctx.gpr[0] = (uint32_t)0;

    // Switch to the new process
    set_current(ctx, pid);

    return;
}

void sys_fork_wait(ctx_t *ctx) {
    pid_t previous_current_pid = current->pid;
    sys_fork(ctx);
    sys_wait(ctx, previous_current_pid, current->pid);
    return;
}

void sys_exit(ctx_t* ctx, int x) {
    // Reset ctx (scheduler will update current with this) and run scheduler
    update_waiters(current->pid, x);
    fix_orphaned_processes(current->pid);

    new_process(current->pid, 0);
    reset_ctx(ctx, current->pid); // is this any differnt from new_process then load_ctx?
    //load_ctx(ctx);

    scheduler(ctx);
    return;
}

int sys_kill(ctx_t* ctx, pid_t pid, uint32_t sig) {
    /* - If pid is positive, then signal sig is sent
    * to the process with the ID specified by pid.
    * - If pid equals 0, then sig is sent to every
    * process in the process group of the calling process.
    * - If pid is less than -1, then sig is sent to every
    * process in the process group whose ID is -pid.
    * - If sig is 0, then no signal is sent, but error
    * checking is still performed; this can be used
    * to check for the existence of a process ID or
    * process group ID.
    */

    if (!active_process(pid)) {
        error("No process exists\n");
        return -1;
    }

    switch (sig) {
        case SIG_TERM: {
            /* Terminates a process immediately. However, this
            * signal can be handled, ignored or caught in
            * code. If the signal is not caught by a process,
            * the process is killed. Also, this is used for
            * graceful termination of a process
            */
            // If killing current process, reset ctx and run scheduler,
            // otherwise update processes ctx so when scheduler next run
            // it will be free
            // TODO Make same as exit, FIX exit
            fix_orphaned_processes(current->pid);
            update_waiters(current->pid, EXIT_FAILURE);
            if (current->pid == pid) {
                reset_ctx(ctx, pid);
                scheduler(ctx);
            } else {
                reset_ctx(&process(pid)->ctx, pid);
            }
            return 0; // Success
        }
        case SIG_QUIT: {
            /*  generates a core dump of the process and also
            * cleans up resources held up by a process. Like
            * SIGINT, this can also be sent from the terminal
            * as input characters. It can be handled, ignored
            * or caught in code.
            */
            // TODO Quit
            error("Unimplemented signal\n");
            return -1; // return error as unimplemented
        }
        default: {
            error("Unknown signal\n");
            return -1; // return error unknown signal
        }
    }
}

void sys_yield(ctx_t* ctx) {
    current->priority.time_left = 0;
    scheduler(ctx);
    return;
}

void sys_exec(ctx_t* ctx, void* x, char* args) {
    // https://linux.die.net/man/3/exec
    // Reset current ctx, update pc to new program, and reload the ctx
    if (x == NULL) {
        error("Invalid program\n");
        sys_exit(ctx, EXIT_FAILURE); // exit forked process
        return;
    }

    reset_ctx(&current->ctx, current->pid);
    current->ctx.pc = (uint32_t)(x);

    // Load the arguments
    current->ctx.gpr[0] = (uint32_t)args;

    load_ctx(ctx);
    return;
}

// Set child processes ppid to 0
// NOTE memory locks, sleeping waters?
void fix_orphaned_processes(pid_t ppid) {
    for (pid_t pid=1; pid <= MAX_PROCESSES; pid++) {
        if (process(pid)->ppid == ppid) {
            process(pid)->ppid = 0;
        }
    }
    return;
}

// Print process info
void ps_stats(pid_t pid) {
    if (!active_process(pid)) {
        error("No process exists\n");
        return;
    }
    pcb_t* p = process(pid);

    char b[1024];
    error("PID "); error(s(b, p->pid));
    error(", PPID "); error(s(b, p->ppid));
    error(", PRIORITY "); error(s(b, p->priority.priority));
    error(", CPU "); error(s(b, p->priority.cpu_burst));
    error(", IO "); error(s(b, p->priority.io_burst));
    error(", ARIVAL TIME "); error(s(b, p->priority.arrival_time));
    error(", TIME LEFT "); error(s(b, p->priority.time_left));
    error("\n");

    return;
}

void sys_ps() {
    for (pid_t pid=1; pid <= MAX_PROCESSES; pid++) {
        if (active_process(pid)) {
            ps_stats(pid);
        }
    }
    return;
}

int sys_share(ctx_t* ctx, void* ptr) {
    int shared = share(current->pid, ptr);
    if (shared == 0) {
        error("Couldn't be shared, something may already be shared\n");
    }
    return shared;
}

int sys_unshare(ctx_t* ctx, void* ptr) {
    int unshared = unshare(current->pid, ptr);
    if (unshared == 0) {
        error("Memory is not shared for unshare\n");
    }
    return unshared;
}

int sys_lock(ctx_t* ctx, void* ptr) {
    int locked = lock(current->pid, ptr);
    if (locked == -1) {
        error("Memory not shared for lock\n");
    }
    return locked;
}

int sys_unlock(ctx_t* ctx, void* ptr) {
    int unlocked = unlock(current->pid, ptr);
    if (unlocked == -1) {
        error("Memory not shared for unlock\n");
    }
    return unlocked;
}

int sys_wait(ctx_t* ctx, pid_t pid, pid_t wait_for_pid) {
    // use the waitings lists and waiter lists
    waiting_t* waiting = get_waiting(pid, wait_for_pid);

    // If not currently waiting for pid, start!
    if (waiting == NULL) {
        waiting = set_waiting(pid, wait_for_pid);
        if (waiting == NULL) {
            error("Couldn't set waiting\n");
            return -2;
        }
    }

    int result = waiting->result;
    if (result == -1) {
        return result;
    }
    // If we have a result then reset
    waiting->pid = 0;
    waiting->result = -1;

    return result;
}
