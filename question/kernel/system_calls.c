#include "system_calls.h"

extern pcb_t* current;

int sys_write(int fd, char* x, int n) {
    // https://linux.die.net/man/2/kill, http://unix.stackexchange.com/questions/80044/how-signals-work-internally

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

int sys_read(int fd, char* x, int n) {
    // TODO use differnt file handlers
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

    // NOTE: Could implement wait system call which waits for child process to complete

    // Copy the process
    pcb_t* new_process = process(pid);
    save_ctx(ctx); // Save the ctx into current
    memcpy(new_process, current, sizeof(pcb_t));

    // Update its pid and ppid
    new_process->pid = pid;
    new_process->ppid = current->pid;

    // Return child pid to parent and 0 to child
    ctx->gpr[0] = (uint32_t)pid;
    new_process->ctx.gpr[0] = (uint32_t)0;

    // Switch to the new process
    set_current(ctx, pid);

    return;
}

void sys_exit(ctx_t* ctx, int x) {
    if (x == EXIT_SUCCESS) {
        // TODO Exit success
    } else if (x == EXIT_FAILURE) {
        // TODO Exit failure
    } else {
        // TODO unkown status code x
    }

    // Reset ctx (scheduler will update current with this) and run scheduler
    reset_ctx(ctx, current->pid);
    fix_orphaned_processes(current->pid);
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
            fix_orphaned_processes(current->pid);
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
    scheduler(ctx);
    return;
}

void sys_exec(ctx_t* ctx, void* x) {
    // https://linux.die.net/man/3/exec
    // Reset current ctx, update pc to new program, and reload the ctx
    if (x == NULL) {
        error("Invalid program\n");
        sys_exit(ctx, EXIT_FAILURE); // exit forked process
        return;
    }

    reset_ctx(&current->ctx, current->pid);
    current->ctx.pc = (uint32_t)(x);
    load_ctx(ctx);
    return;
}

// Set child processes ppid to 0
void fix_orphaned_processes(pid_t ppid) {
    for (pid_t pid=1; pid <= MAX_PROCESSES; pid++) {
        if (process(pid)->ppid == ppid) {
            process(pid)->ppid = 0;
        }
    }
    return;
}
