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
    err("Spawned="); err(ss(b,id)); err(" ");
    int take_fork_off_id = (id+1) % PHILOSOPHERS; // right handed
    //while (!eaten) {
    for (int i=0; i<5;i++) { // TODO remove
        sleep(1); // Thinking
        if (lockm(forks) == -1) {
            exit(EXIT_FAILURE); // Error locking
        }

        err(ss(b,id));err(":Locked, Fork="); err(ss(b, forks[take_fork_off_id])); err("\n");
        if (forks[take_fork_off_id] == 1) { // If there is a fork
            err("philosopher eaten!\n");
            forks[take_fork_off_id] = 0;
            forks[id] = 1;
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
    for (int i=0; i<PHILOSOPHERS-1; i++) {
        forks[i] = 1;
    }

    // Share forks memeory
    sharem(forks);

    err("Spawning philisophers\n");

    // Spawn philosphers
    int philosopher_pids[PHILOSOPHERS] = {0};
    for (int i=0; i<PHILOSOPHERS; i++) {
        pid_t pid = fork();
        if (0 == pid) {
            //exec(&philosopher); // TODO pass id?
            philosopher(i); // TODO will this work?, NOTE exits
            return;
        } else {
            philosopher_pids[i] = pid;
        }
    }

    //lockm(forks);
    //unlockm(forks);

    err("Waiting for responses\n");

    // Wait until they are all complete
    // TODO What if processes finish before get waiting lock on them?
    int result = 0;
    for (int i=0; i<PHILOSOPHERS; i++) {
        err("Waiting for: "); err(ss(b, philosopher_pids[i])); err("\n");
        ps(philosopher_pids[i]);
        // TODO wait_p will wait for the first to complete but not put waiting locks on any of the others until this happens meaning it only waits for the first
        int r = waitp(philosopher_pids[i]);
        err(ss(b, r));
        result = result && r;
    }

    unsharem(forks);

    err("Done=");err(ss(b, result));err("\n");

    exit(result);
}
