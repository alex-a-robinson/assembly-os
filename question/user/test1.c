#include "P5.h"

int number[1];

void forked(int n) {
    err("Starting Forked\n");
    sleep(2);

    err("Waiting for lock\n");
    lockm(number);
    if (number[0] == 1) {
        err("Setting to 2\n");
        number[0] = 2;
    } else {
        err("Waiting until 1\n");
    }
    unlockm(number);

    exit( EXIT_SUCCESS );
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

    sleep(5);

    err("Waiting for lock-\n");
    lockm(number);
    err("Setting to 1\n");
    number[0] = 1;
    unlockm(number);

    waitp(pid);
    unsharem(number);

    char b[1024];
    err("Number ");err(ss(b, number[0]));err("\n");

    exit( EXIT_SUCCESS );
}
