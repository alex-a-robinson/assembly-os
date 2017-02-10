#include "hilevel.h"

// Programs
extern void main_console();
extern pcb_t* current;
int BURSTS = 0;

void hilevel_handler_rst(ctx_t* ctx) {
    // Initialise PCBs representing processes
    init_pcbs();

    /* Once the PCBs are initialised, we (arbitrarily) select one to be
    * restored (i.e., executed) when the function then returns.
    */

    // Load console as first process
    process(1)->ctx.pc = (uint32_t)(&main_console);
    current = process(1);
    load_ctx(ctx);

    /* Configure the mechanism for interrupt handling by
    *
    * - configuring timer st. it raises a (periodic) interrupt for each
    *   timer tick,
    * - configuring GIC st. the selected interrupts are forwarded to the
    *   processor via the IRQ interrupt signal, then
    * - enabling IRQ interrupts.
    */

    TIMER0->Timer1Load = 0x00020000; // 0x00020000 ~ 0.125 seconds, select period 0x00100000 = 2^20 ticks ~= 1 sec
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
        BURSTS++;
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
        case SYS_FORK: {
            sys_fork(ctx);
            break;
        }
        case SYS_EXIT: {
            int x = (int)(ctx->gpr[0]); // status code
            sys_exit(ctx, x);
            break;
        }
        case SYS_KILL: {
            pid_t pid = (pid_t)(ctx->gpr[0]);
            uint32_t x = (uint32_t)(ctx->gpr[1]); // signal
            int status = sys_kill(ctx, pid, x);
            ctx->gpr[0] = (uint32_t)status;
            break;
        }
        case SYS_WRITE: {
            int fd = (int)(ctx->gpr[0]); // write to file descriptor
            char* x = (char*)(ctx->gpr[1]); // read from
            int n = (int)(ctx->gpr[2]); // number of bytes
            int bytes_written = sys_write(fd, x, n);
            ctx->gpr[0] = (uint32_t)bytes_written;
            break;
        }
        case SYS_READ: {
            int fd = (int)(ctx->gpr[0]); // read from file descriptor
            char* x = (char*)(ctx->gpr[1]); // write into
            int n = (int)(ctx->gpr[2]); // number of bytes
            int bytes_read = sys_read(fd, x, n);
            ctx->gpr[0] = (uint32_t)bytes_read;
            break;
        }
        case SYS_YIELD: {
            sys_yield(ctx);
            break;
        }
        case SYS_EXEC: {
            void* x = (void*)(ctx->gpr[0]); // start executing program at address x e.g. &main_P3
            sys_exec(ctx, x);
            break;
        }
        case SYS_PS: {
            pid_t pid = (pid_t)(ctx->gpr[0]);
            if (pid == 0) {
                sys_ps();
            } else {
                ps_stats(pid);
            }
            break;
        }
        case SYS_SHARE: {
            void* ptr = (void*)(ctx->gpr[0]);
            int r = sys_share(ctx, ptr);
            ctx->gpr[0] = r;
            break;
        }
        case SYS_UNSHARE: {
            void* ptr = (void*)(ctx->gpr[0]);
            int r = sys_unshare(ctx, ptr);
            ctx->gpr[0] = r;
            break;
        }
        case SYS_LOCK: {
            void* ptr = (void*)(ctx->gpr[0]);
            int r = sys_lock(ctx, ptr);
            ctx->gpr[0] = r;
            break;
        }
        case SYS_UNLOCK: {
            void* ptr = (void*)(ctx->gpr[0]);
            int r = sys_unlock(ctx, ptr);
            ctx->gpr[0] = r;
            break;
        }
        case SYS_WAIT: {
            pid_t pid = (void*)(ctx->gpr[0]);
            int r = sys_wait(ctx, pid);
            ctx->gpr[0] = r;
            break;
        }
        default: { // 0x?? => unknown/unsupported
            error("unknown/unsupported system call\n");
            break;
        }
    }

    return;
}
