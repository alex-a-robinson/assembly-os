#include "prog.h"

void main_prog_ps(char* args) {
    pid_t pid = 0;
    if (strlen(args) != 0) {
        pid = atoi(strtok(args, " "));
    }
    ps(pid);
    exit(EXIT_SUCCESS);
}
