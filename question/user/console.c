#include "console.h"

void gets(char* x, int n) {
    for (int i = 0; i < n; i++) {
        x[i] = PL011_getc(UART1, true); // UART1?

        if (x[i] == '\x0A') {
            x[i] = '\x00';
            break;
        }
    }
}

/* Since we lack a *real* loader (as a result of lacking a storage
* medium to store program images), the following approximates one:
* given a program name, from the set of programs statically linked
* into the kernel image, it returns a pointer to the entry point.
*/

extern void main_P3();
extern void main_P4();
extern void main_P5();
extern void main_dp();

void* load(char* x) {
    if (0 == strcmp(x, "P3")) {
        return &main_P3;
    } else if (0 == strcmp(x, "P4")) {
        return &main_P4;
    } else if (0 == strcmp(x, "P5")) {
        return &main_P5;
    } else if (0 == strcmp(x, "DP")) {
        return &main_dp;
    }

    return NULL;
}

/* The behaviour of the console process can be summarised as an
* (infinite) loop over three main steps, namely
*
* 1. write a command prompt then read a command,
* 2. split the command into space-separated tokens using strtok,
* 3. execute whatever steps the command dictates.
*/

void main_console() {
    char* p, x[1024];

    while (1) {
        //write("shell$ ", 7);
        write(STDERR_FILENO, "shell$ ", 7);
        gets(x, 1024);
        //read(STDIN_FILENO, x, 1024);
        p = strtok(x, " ");

        if (0 == strcmp(p, "fork")) {
            pid_t pid = fork();

            if (0 == pid) {
                void* addr = load(strtok(NULL, " "));
                exec(addr);
            }
        } else if (0 == strcmp(p, "kill")) {
            pid_t pid = atoi(strtok(NULL, " "));
            int s = atoi(strtok(NULL, " "));

            kill(pid, s);
        } else if (0 == strcmp(p, "ps")) {
            pid_t pid = atoi(strtok(NULL, " "));
            ps(pid);
        } else {
            err("unknown command\n");
        }
    }

    exit(EXIT_SUCCESS);
}
