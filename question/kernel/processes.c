#include "processes.h"

pcb_t pcb[MAX_PROCESSES], *current = NULL;
extern int BURSTS;

// Stacks
extern uint32_t tos_P1;

// TODO dynamically by adding to pointers
uint32_t sps[MAX_PROCESSES];

// Return process
pcb_t* process(pid_t pid) {
    return &pcb[pid-1];
}

void init_stack_pointers() {
    for (int i=0; i < MAX_PROCESSES; i++) {
        sps[i] = (uint32_t)(((uint32_t)(&tos_P1)) + PROGRAM_STACK_SIZE * i);
    }
}

pcb_t* parent_process(pid_t pid) {
    pcb_t* p = process(pid);
    if (p->ppid == 0) {
        return NULL;
    }
    return process(p->ppid);
}

void reset_priority(pcb_t* p) {
    p->priority.priority = 0;
    p->priority.io_burst = 0;
    p->priority.cpu_burst = 0;
    p->priority.arrival_time = BURSTS;
    p->priority.time_left = 0;
    p->priority.waiting_time = 0;
    p->priority.process_type = BACKGROUND;
}

void reset_ctx(ctx_t* ctx, pid_t pid) {
    /* The CPSR value of 0x50 means the processor is switched into USR
    *   mode, with IRQ interrupts enabled, and
    * - the PC and SP values matche the entry point and top of stack.
    */
    ctx->pc = (uint32_t)0;
    ctx->cpsr = 0x50;
    memset(&ctx->gpr, (uint32_t)0, sizeof(ctx->gpr));
    ctx->sp = sps[pid-1];
    ctx->lr = (uint32_t)0;

    return;
}

pcb_t* new_process(pid_t pid, pid_t ppid) {
    pcb_t* p = process(pid);
    memset(p, 0, sizeof(pcb_t));
    p->pid = pid;
    p->ppid = ppid;

    // Set priority & ctx
    reset_priority(p);
    reset_ctx(&p->ctx, pid); // NOTE doens't execute past here?

    return p;
}

// Returns 1 if process is active else 0
int active_process(pid_t pid) {
    return ((process(pid) != NULL) && (process(pid)->ctx.pc != 0));
}

void init_pcbs() {
    init_stack_pointers();
    for (pid_t pid=1; pid <= MAX_PROCESSES; pid++) {
        new_process(pid, 0);
    }
}

// Find a pid which is free
pid_t free_pid() {
    for (pid_t pid=1; pid <= MAX_PROCESSES; pid++) {
        if (!active_process(pid)) {
            return pid;
        }
    }

    return -1; // indicating an error
}

// Load the current ctx into the ctx
void load_ctx(ctx_t* ctx) {
    memcpy(ctx, &current->ctx, sizeof(ctx_t));
    return;
}

// Save the ctx into the current
void save_ctx(ctx_t* ctx) {
    memcpy(&current->ctx, ctx, sizeof(ctx_t));
    return;
}

// Switch current process
void set_current(ctx_t* ctx, pid_t pid) {
    // No need to switch if current
    if (current->pid == pid) {
        return;
    }
    save_ctx(ctx);
    current = process(pid);
    load_ctx(ctx);
    return;
}

// Returns the location of the shared pointer (either own or parents) or NULL if not accessible
shared_t* is_shared(pid_t pid, void* ptr) {
    pcb_t* p = process(pid);
    if (p->shared.ptr == ptr) {
        return &p->shared;
    } else if (process(p->ppid) != NULL && process(p->ppid)->shared.ptr == ptr) {
        return &process(p->ppid)->shared;
    }

    return NULL;
}

int share(pid_t pid, void* ptr) {
    pcb_t* p = process(pid);
    if (p->shared.ptr != NULL) {
        return 0; // Something already shared, error
    }
    p->shared.ptr = ptr;
    return 1;
}

int unshare(pid_t pid, void* ptr) {
    pcb_t* p = process(pid);
    if (p->shared.ptr != ptr) {
        return 0; // ptr is not shared, error
    }
    p->shared.ptr = NULL;
    return 1;
}

int lock(pid_t pid, void* ptr) {
    shared_t* shared = is_shared(pid, ptr);

    if (shared == NULL) {
        return -1;
    }

    if (shared->locked) {
        return 0;
    } else {
        shared->locked = 1;
        return 1;
    }
}

int unlock(pid_t pid, void* ptr) {
    shared_t* shared = is_shared(pid, ptr);

    if (shared == NULL) {
        return -1;
    }

    if (shared->locked) {
        shared->locked = 0;
        return 1;
    } else {
        return 0;
    }
}

waiting_t* get_waiting(pid_t pid, pid_t waiting_pid) {
    for (int i=0; i<MAX_WAITING; i++) {
        if (process(pid)->waiting[i].pid == waiting_pid) {
            return &process(pid)->waiting[i];
        }
    }
    return NULL;
}

// Update all processes waiting for the process of pid to finish
void update_waiters(pid_t pid, int result) {
    for (int i=0; i<MAX_WAITERS; i++) {
        if (process(pid)->waiters[i].pid != 0) {
            get_waiting(process(pid)->waiters[i].pid, pid)->result = result;
            process(pid)->waiters[i].pid = 0; // clear self
        }
    }
}

waiting_t* set_waiting(pid_t pid, pid_t waiting_pid) {
    // Set up the waiting & waiter objects
    waiting_t waiting;
    waiting.pid = waiting_pid;
    waiting.result = -1;

    waiter_t waiter;
    waiter.pid = pid;

    if (!active_process(waiting_pid)) {
        return NULL; // Process is not active
    }

    // Find a free waiters slot
    int set = 0;
    for (int i=0; i<MAX_WAITERS; i++) {
        if (process(waiting_pid)->waiters[i].pid == 0) {
            process(waiting_pid)->waiters[i] = waiter;
            set = 1;
            break;
        }
    }

    // Check for errors
    if (!set) {
        return NULL; // Error, max waiting
    }

    // Find a free waiting slot
    for (int i=0; i<MAX_WAITING; i++) {
        if (process(pid)->waiting[i].pid == 0) {
            process(pid)->waiting[i] = waiting;
            return &process(pid)->waiting[i];
        }
    }

    return NULL;
}

// Add a file descriptor from a processes open file table
int add_process_fd(pid_t pid, int fd) {
    fd_proc_table_t* fdtable;
    fdtable = &process(pid)->fd_table;

    // Check we can open another file
    if (fdtable->count >= MAX_OPEN_FILES) {
        return -1;
    }

    fdtable->open[fdtable->count] = fd;
    fdtable->count++;
    return 0;
}

// Removes a file descriptor from a processes open file table
int remove_process_fd(pid_t pid, int fd) {
    fd_proc_table_t* fdtable;
    fdtable = &process(pid)->fd_table;

    int index = -1;
    for (int i=0; i < fdtable->count; i++) {
        if (fdtable->open[i] == fd) {
            index = i;
            break;
        }
    }

    // File not open
    if (index == -1) {
        return -1;
    }

    // Update number of open files
    fdtable->count--;

    // Shift array left one, therefore deleting the file descriptor
    memmove(&fdtable->open[index+1], &fdtable->open[index], (fdtable->count-index) * sizeof(int));

    return 0;
}

// Returns 1 if process contains file descriptor in open files table
int proc_fd_open(pid_t pid, int file_descriptor_id) {
    fd_proc_table_t* fdtable = &process(pid)->fd_table;
    for (int i=0; i < fdtable->count; i++) {
        if (fdtable->open[i] == file_descriptor_id) {
            return 1;
        }
    }
    return -1;
}
