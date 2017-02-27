#include "echo.h"

void main_echo(char* args) {
    write(STDOUT_FILENO, args, strlen(args));
}
