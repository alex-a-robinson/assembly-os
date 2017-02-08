#include "hilevel.h"

pcb_t pcb[MAX_PROCESSES], *current = NULL;

// Stacks
extern uint32_t tos_P1;
extern uint32_t tos_P2;
extern uint32_t tos_P3;
uint32_t sps[] = {(uint32_t)(&tos_P1), (uint32_t)(&tos_P2), (uint32_t)(&tos_P3)};

// Programs
extern void main_console();

// Find a pid which is free, 0 for error
pid_t free_pid() {
    for (int p_index=0; p_index < MAX_PROCESSES; p_index++) {
        if (pcb[p_index].ctx.pc == 0) {
            return p_index + 1; // pid
        }
    }

    return -1; // indicating an error
}

// Set child processes ppid to 0
void fix_orphaned_processes(pid_t ppid) {
    for (int p_index=0; p_index < MAX_PROCESSES; p_index++) {
        if (pcb[p_index].ppid == ppid) {
            pcb[p_index].ppid = 0;
        }
    }
    return;
}

// Implements scheduling algorithm, NOTE loops if all processes finished
pid_t next_pid() {
    int p_index = current->pid-1;
    do {
        p_index = (p_index + 1) % MAX_PROCESSES;
    } while (pcb[p_index].ctx.pc == 0); // Infinite if all processes finsiehd

    return p_index;
}

// Switch current process
void switch_to_pid(ctx_t* ctx, pid_t pid) {
    // No need to switch if current
    if (current->pid == pid) {
        return;
    }
    memcpy(&current->ctx, ctx, sizeof(ctx_t)); // save current state
    current = &pcb[pid-1];
    memcpy(ctx, &current->ctx, sizeof(ctx_t)); // update new state
    return;
}

// Load the current ctx into the ctx
void reload_current_ctx(ctx_t* ctx) {
    memcpy(ctx, &current->ctx, sizeof(ctx_t));
    return;
}

// Select the next process and switch to it
void scheduler(ctx_t* ctx) {
    pid_t pid = next_pid();
    switch_to_pid(ctx, pid);
    return;
}

void reset_ctx(ctx_t* ctx, pid_t pid) {//, void* sp) {
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

void hilevel_handler_rst(ctx_t* ctx) {
    // Initialise PCBs representing processes
    for (int i=0; i < MAX_PROCESSES; i++) {
        memset(&pcb[i], 0, sizeof(pcb_t));
        pcb[i].pid = i+1;
        pcb[i].ppid = 0; // Default to 0 ppid
        reset_ctx(&pcb[i].ctx, pcb[i].pid);
    }

    /* Once the PCBs are initialised, we (arbitrarily) select one to be
    * restored (i.e., executed) when the function then returns.
    */

    // Load console as first process
    pcb[0].ctx.pc = (uint32_t)(&main_console);
    current = &pcb[0];
    reload_current_ctx(ctx);

    /* Configure the mechanism for interrupt handling by
    *
    * - configuring timer st. it raises a (periodic) interrupt for each
    *   timer tick,
    * - configuring GIC st. the selected interrupts are forwarded to the
    *   processor via the IRQ interrupt signal, then
    * - enabling IRQ interrupts.
    */

    TIMER0->Timer1Load = 0x00100000; // select period = 2^20 ticks ~= 1 sec
    TIMER0->Timer1Ctrl = 0x00000002; // select 32-bit   timer
    TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
    TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
    TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

    GICC0->PMR = 0x000000F0; // unmask all            interrupts
    GICD0->ISENABLER1 |= 0x00000010; // enable timer          interrupt
    GICC0->CTLR = 0x00000001; // enable GIC interface
    GICD0->CTLR = 0x00000001; // enable GIC distributor

    int_enable_irq();

    return;
}

void hilevel_handler_irq(ctx_t* ctx) {
    // Step 2: read  the interrupt identifier so we know the source.

    uint32_t id = GICC0->IAR;

    // Step 4: handle the interrupt, then clear (or reset) the source.

    if (id == GIC_SOURCE_TIMER0) { // Timer
        scheduler(ctx);
        TIMER0->Timer1IntClr = 0x01;
    }

    // Step 5: write the interrupt identifier to signal we're done.

    GICC0->EOIR = id;

    return;
}

void hilevel_handler_svc(ctx_t* ctx, uint32_t id) {
    /* Based on the identified encoded as an immediate operand in the
    * instruction,
    *
    * - read  the arguments from preserved usr mode registers,
    * - perform whatever is appropriate for this system call,
    * - write any return value back to preserved usr mode registers.
    */

    switch (id) {
        case SYS_FORK: { // https://linux.die.net/man/2/fork
            // NOTE what to do about file descriptors?
            pid_t pid = free_pid();

            // If no pid, return an error
            if (pid == 0) {
                current->ctx.gpr[0] = -1;
                break;
            }

            // NOTE: Could implement wait system call which waits for child process to complete

            // Copy the process
            pcb_t* new_process = &pcb[pid-1];
            memcpy(new_process, current, sizeof(pcb_t));

            // Update its pid and ppid
            new_process->pid = pid;
            new_process->ppid = current->pid;

            // Return child pid to parent and 0 to child
            current->ctx.gpr[0] = pid;
            new_process->ctx.gpr[0] = 0;

            // Switch to the new process
            switch_to_pid(ctx, pid);
            break;
        }
        case SYS_EXIT: {
            int x = (int)(ctx->gpr[0]); // status code

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
            break;
        }
        case SYS_KILL: { // https://linux.die.net/man/2/kill, http://unix.stackexchange.com/questions/80044/how-signals-work-internally
            pid_t pid = (pid_t)(ctx->gpr[0]);
            int x = (int)(ctx->gpr[1]); // signal

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

            if (x == SIG_TERM) { // Imediately terminate
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
                    reset_ctx(&pcb[pid-1].ctx, pid);
                }
                ctx->gpr[0] = 0; // Success
            } else if (x == SIG_QUIT) { // Let process handle
                /*  generates a core dump of the process and also
                * cleans up resources held up by a process. Like
                * SIGINT, this can also be sent from the terminal
                * as input characters. It can be handled, ignored
                * or caught in code.
                */
                // TODO Quit
                ctx->gpr[0] = -1; // return error as unimplemented
            } else {
                ctx->gpr[0] = -1; // return error unknown signal
            }

            break;
        }
        case SYS_WRITE: {
            int fd = (int)(ctx->gpr[0]); // write to file descriptor
            char* x = (char*)(ctx->gpr[1]); // read from
            int n = (int)(ctx->gpr[2]); // number of bytes

            // TODO use differnt file handlers e.g. stderr

            for (int i = 0; i < n; i++) {
                PL011_putc(UART0, *x++, true);
            }

            ctx->gpr[0] = n; // bytes written
            break;
        }
        // case SYS_READ: {
        //     int fd = (int)(ctx->gpr[0]); // read from file descriptor
        //     char* x = (char*)(ctx->gpr[1]); // write into
        //     int n = (int)(ctx->gpr[2]); // number of bytes
        //     // TODO use differnt file handlers
        //
        //     // TODO test this works
        //     for (int i = 0; i < n; i++) {
        //         x[i] = PL011_getc(UART0, true);
        //
        //         if (x[i] == '\x0A') {
        //             x[i] = '\x00';
        //             break;
        //         }
        //     }
        //
        //     ctx->gpr[0] = n; // bytes read
        //     break;
        // }
        case SYS_YIELD: {
            scheduler(ctx);
            break;
        }
        case SYS_EXEC: { // https://linux.die.net/man/3/exec
            void* x = (void*)(ctx->gpr[0]); // start executing program at address x e.g. &main_P3

            // Reset current ctx, update pc to new program, and reload the ctx
            reset_ctx(&current->ctx, current->pid);
            current->ctx.pc = (uint32_t)(x);
            reload_current_ctx(ctx);
            break;
        }
        default: { // 0x?? => unknown/unsupported
            break;
        }
    }

    return;
}
