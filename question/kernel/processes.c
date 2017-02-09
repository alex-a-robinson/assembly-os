#include "processes.h"

pcb_t pcb[MAX_PROCESSES], *current = NULL;

// Stacks
extern uint32_t tos_P1;
extern uint32_t tos_P2;
extern uint32_t tos_P3;
uint32_t sps[] = {(uint32_t)(&tos_P1), (uint32_t)(&tos_P2), (uint32_t)(&tos_P3)};

// Return process
pcb_t* process(pid_t pid) {
    return &pcb[pid-1];
}

// Returns 1 if process is active else 0
int active_process(pid_t pid) {
    return (process(pid)->ctx.pc != 0);
}

void init_pcbs() {
    for (pid_t pid=1; pid <= MAX_PROCESSES; pid++) {
        memset(process(pid), 0, sizeof(pcb_t));
        process(pid)->pid = pid;
        process(pid)->ppid = 0; // Default to 0 ppid
        reset_ctx(&process(pid)->ctx, pid);
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
