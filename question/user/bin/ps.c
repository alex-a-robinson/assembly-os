#include "prog.h"

void main_prog_ps(char* args) {
    pid_t pid = atoi(strtok(args, " "));
    ps(pid);
    exit(EXIT_SUCCESS);
}
