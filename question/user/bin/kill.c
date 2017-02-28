#include "prog.h"

void main_prog_kill(char* args) {
    pid_t pid = atoi(strtok(args, " "));
    int s = atoi(strtok(NULL, " "));

    kill(pid, s);
    exit(EXIT_SUCCESS);
}
