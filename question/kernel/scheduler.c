#include "scheduler.h"

extern pcb_t* current;

// Implements scheduling algorithm
pid_t next_pid() {
    pid_t pid = current->pid;
    for (int i=0; i < MAX_PROCESSES; i++) {
        pid = (pid % MAX_PROCESSES) + 1;
        if (active_process(pid)) {
            return pid;
        }
    }

    return -1;
}

// Select the next process and switch to it
void scheduler(ctx_t* ctx) {
    pid_t pid = next_pid();
    if (pid == -1) {
        // TODO scope error("No processes to schedule");
        return;
    }
    set_current(ctx, pid);
    return;
}
