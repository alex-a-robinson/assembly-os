#include "dinning_philosophers.h"

/* TODO
* - Implement sleep
* - Implement lock
* - Implement unlock
* - Implement shared memory
*
* NOTE: Access to pointer is security violation? Pass with it size so can only access some amount
*/

int forks[PHILOSOPHERS];

void philosopher(int id) {
    int eaten = 0;
    char b[1024];
    puts(ss(b,id));puts("] Spawned\n");
    int take_fork_off_id = (id+1) % PHILOSOPHERS; // right handed
    //while (!eaten) {
    for (int i=0; i<5;i++) { // TODO remove
        sleep(1); // Thinking

        if (lockm(forks) == -1) {
            exit(EXIT_FAILURE); // Error locking
        }

        puts(ss(b,id));puts("] Locked, Fork="); puts(ss(b, forks[take_fork_off_id])); puts("\n");
        if (forks[take_fork_off_id] == 1) { // If there is a fork
            puts(ss(b,id));puts("] Eaten!\n");
            forks[take_fork_off_id] = 0;
            forks[id] = 1; // LOGIC?
            sleep(1); // Eating
            eaten = 1;
        }
        unlockm(forks);
        if (eaten) {break;} // TODO remove
    }

    exit(EXIT_SUCCESS);
}

void main_dp() {
    char b[1024];

    err("Iniated forks\n");
    // Init array (all to 1 except 1)
    // for (int i=0; i<PHILOSOPHERS-1; i++) {
    //     forks[i] = 1;
    // }

    // Share forks memeory
    sharem(forks);

    err("Spawning philisophers\n");

    // Spawn philosphers
    int philosopher_pids[PHILOSOPHERS] = {0};
    for (int i=0; i<PHILOSOPHERS; i++) {
        pid_t pid = fork();
        if (0 == pid) {
            philosopher(i);
            return;
        } else {
            philosopher_pids[i] = pid;
        }
    }

    // Init array (all to 1 except last)
    lockm(forks);
    for (int i=0; i<PHILOSOPHERS; i++) { // TODO should be PHILOSOPHERS-1
        forks[i] = 1;
    }
    unlockm(forks);

    // Put non blocking wait requests in
    for (int i=0; i<PHILOSOPHERS; i++) {
        waitpnb(philosopher_pids[i]);
    }

    err("Waiting for responses\n");

    // Wait until they are all complete
    // TODO What if processes finish before get waiting lock on them?
    int result = 0;
    for (int i=0; i<PHILOSOPHERS; i++) {
        err("Waiting for: "); err(ss(b, philosopher_pids[i])); err("\n");
        int r = waitp(philosopher_pids[i]);
    }

    unsharem(forks);

    err("Done=");err(ss(b, result));err("\n");

    exit(result);
}
