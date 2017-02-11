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

void philosopher() {//int id) {
    int eaten = 0;
    err("Spawned!\n");
    //int take_fork_off_id = (id+1) % PHILOSOPHERS; // right handed
    while (!eaten) {
        sleep(1); // Thinking
        lockm(forks); // blocking
        err("p");
        //forks[take_fork_off_id] = 0;
        //forks[id] = 1;
        sleep(1); // Eating
        eaten = 1;
        unlockm(forks); // blocking
    }

    err("philosopher eaten!\n");

    exit(EXIT_SUCCESS);
}

void main_dp() {
    // Init array (all to 1 except 1)
    for (int i=0; i<PHILOSOPHERS-1; i++) {
        forks[i] = 1;
    }

    err("Iniated forks\n");

    // Share forks memeory
    sharem(forks);

    err("Spawning philisophers\n");

    // Spawn philosphers
    int philosopher_pids[PHILOSOPHERS] = {0};
    for (int i=0; i<PHILOSOPHERS; i++) {
        pid_t pid = fork();
        if (0 == pid) {
            //exec(&philosopher); // TODO pass id?
            philosopher(); // TODO will this work?, NOTE exits
        } else {
            philosopher_pids[i] = pid;
        }
    }

    err("Waiting for responses\n");

    // Wait until they are all complete
    // TODO What if processes finish before get waiting lock on them?
    int result = 0;
    for (int i=0; i<PHILOSOPHERS; i++) {
        result = result && waitp(philosopher_pids[i]);
    }

    err("Done!\n");

    exit(result);
}
