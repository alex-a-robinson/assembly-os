#include "scheduler.h"

extern pcb_t* current;

int active_process_count() {
    int count = 0;
    for (pid_t pid=0; pid <= MAX_PROCESSES; pid++) {
        if (active_process(pid)) {
            count++;
        }
    }

    return count;
}

// Returns pid of process_type with max waiting time
pid_t next_pid_with_type(int process_type) {
    int max_waiting_time = -1;
    pid_t next_pid = -1;
    pid_t pid;
    for (int i=0; i < MAX_PROCESSES; i++) {
        pid = ((current->pid + i) % MAX_PROCESSES) + 1;
        if (!active_process(pid)) {
            continue;
        }

        priority_t priority = process(pid)->priority;
        if (priority.process_type == process_type && priority.waiting_time > max_waiting_time) {
            next_pid = pid;
            max_waiting_time = priority.waiting_time;
        }
    }

    return next_pid;
}

// Implements scheduling algorithm
pid_t next_pid() {
    // Toggle between process types
    int next_process_type;
    if (current->priority.process_type == BACKGROUND) {
        next_process_type = INTERACTIVE;
    } else {
        next_process_type = BACKGROUND;
    }

    pid_t pid = next_pid_with_type(next_process_type);

    // If no processes with that type, use the current type instead
    if (pid == -1 || (pid == current->pid && active_process_count() > 1)) {
        pid = next_pid_with_type(current->priority.process_type);
    }

    return pid;
}

// void calculate_priorities() {
//     pid_t pid;
//     for (int i=0; i < MAX_PROCESSES; i++) {
//         pid = ((current->pid + i) % MAX_PROCESSES) + 1;
//         if (!active_process(pid)) {
//             continue;
//         }
//
//         pcb_t* proc = process(pid);
//         priority_t* priority = &proc->priority;
//
//         // Determin process_type by checking io ratio
//         float io_ratio = ((float)priority->cpu_burst / priority->io_burst);
//         if (priority->io_burst) {//(io_ratio > IO_INTERACTIVE_THRESHOLD) {
//             priority->process_type = INTERACTIVE;
//         } else {
//             priority->process_type = BACKGROUND;
//         }
//     }
//     return;
// }

// Update the waiting times for all processes
void update_waiting() {
    pid_t pid;
    for (int i=0; i < MAX_PROCESSES; i++) {
        pid = ((current->pid + i) % MAX_PROCESSES) + 1;
        if (!active_process(pid) || pid == current->pid) {
            continue;
        }
        process(pid)->priority.waiting_time++;
    }
}

// Select the next process and switch to it
void scheduler(ctx_t* ctx) {
    update_waiting();
    if (current->priority.time_left > 0) {
        current->priority.time_left--;
        return;
    }
    //calculate_priorities();
    pid_t pid = next_pid();
    if (pid == -1) {
        // TODO scope error("No processes to schedule");
        return;
    }
    set_current(ctx, pid);
    current->priority.waiting_time = 0;
    if (current->priority.process_type == INTERACTIVE) {
        current->priority.time_left = 1; // TODO small time slice
    } else {
        current->priority.time_left = 6; // NOTE larger time slice
    }

    current->priority.cpu_burst++;
    return;
}
