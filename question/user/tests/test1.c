#include "test1.h"

int number[1];

void forked(int n) {
    puts("Starting Forked\n");

    char b[1024];
    while (1) {
        sleep(1);
        puts("Waiting for lock\n");
        lockm(number);
        puts(ss(b, number[0]));
        if (number[0] == 1) {
            puts("Setting to 2\n");
            number[0] = 2;
            exit(EXIT_SUCCESS);
        } else {
            puts("Waiting until 1\n");
        }
        unlockm(number);
    }

    exit( EXIT_FAILURE );
}

void main_TEST1() {
    err("Starting TEST1\n");

    number[0] = 0;

    sharem(number);

    pid_t pid = fork();
    if (0 == pid) {
        forked(2);
        return;
    }

    err("Waiting for lock-\n");
    lockm(number);
    err("Setting to 1\n");
    number[0] = 1;
    unlockm(number);
    err("Unlocked-\n");

    waitp(pid);
    err("Unshared-\n");
    unsharem(number);

    char b[1024];
    err("Number ");err(ss(b, number[0]));err("\n");

    exit( EXIT_SUCCESS );
}
