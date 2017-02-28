#include "prog.h"

void main_prog_echo(char* args) {
    write(STDOUT_FILENO, args, strlen(args));
    exit(EXIT_SUCCESS);
}
