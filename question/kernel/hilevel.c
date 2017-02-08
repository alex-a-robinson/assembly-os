#include "hilevel.h"

pcb_t pcb[MAX_PROCESSES], *current = NULL;

// Stacks
extern uint32_t tos_P1;
extern uint32_t tos_P2;
extern uint32_t tos_P3;
uint32_t sps[] = {(uint32_t)(&tos_P1), (uint32_t)(&tos_P2), (uint32_t)(&tos_P3)};

// Programs
extern void main_console();
extern void main_P3(); // TODO REMOVE
extern void main_P4(); // TODO REMOVE
extern void main_P5(); // TODO REMOVE

void scheduler(ctx_t* ctx) {
    memcpy(&current->ctx, ctx, sizeof(ctx_t));

    // Loop through all processes until finds incomplete one
    int p_index = current->pid-1;
    do {
        p_index = (p_index + 1) % MAX_PROCESSES;
        current = &pcb[p_index];
    } while (current->ctx.pc == 0); // Infinite if all processes finsiehd

    memcpy(ctx,&current->ctx, sizeof(ctx_t));

    return;
}

void reset_ctx(ctx_t* ctx, uint32_t pid) {//, void* sp) {
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
        reset_ctx(&pcb[i].ctx, pcb[i].pid);
    }

    /* Once the PCBs are initialised, we (arbitrarily) select one to be
    * restored (i.e., executed) when the function then returns.
    */

    // Load console as first process
    pcb[1].ctx.pc = (uint32_t)(&main_console);
    pcb[0].ctx.pc = (uint32_t)(&main_P3); // TODO rmeove
    //pcb[1].ctx.pc = (uint32_t)(&main_P4); // TODO rmeove
    //pcb[2].ctx.pc = (uint32_t)(&main_P5); // TODO rmeove
    current = &pcb[0];
    memcpy(ctx, &current->ctx, sizeof(ctx_t));

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

    /* Configure the mechanism for interrupt handling by
    *
    * - configuring UART st. an interrupt is raised every time a byte is
    *   subsequently received,
    * - configuring GIC st. the selected interrupts are forwarded to the
    *   processor via the IRQ interrupt signal, then
    * - enabling IRQ interrupts.
    */

    UART0->IMSC       |= 0x00000010; // enable UART    (Rx) interrupt
    UART0->CR          = 0x00000301; // enable UART (Tx+Rx)


    GICC0->PMR = 0x000000F0; // unmask all            interrupts

    GICD0->ISENABLER1 |= 0x00001000; // enable UART    (Rx) interrupt
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
    } else if (id == GIC_SOURCE_UART0) { // Keyboard?
        uint8_t x = PL011_getc( UART0, true );

        // TODO Testing by putting it on the screen
        PL011_putc( UART0, 'K',                      true );
        PL011_putc( UART0, '<',                      true );
        PL011_putc( UART0, itox( ( x >> 4 ) & 0xF ), true );
        PL011_putc( UART0, itox( ( x >> 0 ) & 0xF ), true );
        PL011_putc( UART0, '>',                      true );

        UART0->ICR = 0x10;
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
        case SYS_FORK: {
            // TODO
            /* Upon successful completion, fork() returns a value of 0
            * to the child process and returns the process ID of the
            * child process to the parent process. Otherwise, a value
            * of -1 is returned to the parent process, no child process
            * is created, and the global variable [errno][1] is set to
            * indicate the error. */
            ctx->gpr[0] = 0; // TODO See above note
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
            scheduler(ctx);
            break;
        }
        case SYS_KILL: {
            pid_t pid = (pid_t)(ctx->gpr[0]);
            int x = (int)(ctx->gpr[1]); // signal
            // TODO

            if (x == SIG_TERM) {
                // TODO Terminate
            } else if (x == SIG_QUIT) {
                // TODO Quit
            } else {
                // TODO Unknown signal x
            }

            // If killing current proccess, reset ctx and run scheduler,
            // otherwise update proccesses ctx so when scheduler next run
            // it will be free
            if (current->pid == pid) {
                reset_ctx(ctx, pid);
                scheduler(ctx);
            } else {
                reset_ctx(&pcb[pid-1].ctx, pid);
            }

            ctx->gpr[0] = 0; // TODO result of exit meaningless?
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
        case SYS_READ: {
            int fd = (int)(ctx->gpr[0]); // read from file descriptor
            char* x = (char*)(ctx->gpr[1]); // write into
            int n = (int)(ctx->gpr[2]); // number of bytes
            // TODO use differnt file handlers

            // TODO test this works
            for (int i = 0; i < n; i++) {
                x[i] = PL011_getc(UART1, true);

                if (x[i] == '\x0A') {
                    x[i] = '\x00';
                    break;
                }
            }

            ctx->gpr[0] = n; // bytes read
            break;
        }
        case SYS_YIELD: {
            scheduler(ctx);
            break;
        }
        case SYS_EXEC: {
            void* x = (void*)(ctx->gpr[0]); // start executing program at address x e.g. &main_P3
            // TODO
            for (int i=0; i < MAX_PROCESSES; i++) {
                if (pcb[i].ctx.pc == 0) {
                    pcb[i].ctx.pc = (uint32_t)(x);
                    return; // Successfully dded to queue
                }
            }
            return; // Failed to add to queue

            break;
        }
        default: { // 0x?? => unknown/unsupported
            break;
        }
    }

    return;
}
