#include "utilities.h"

void error(char* msg) {
    strcat(msg, "\n");
    sys_write(STDERR_FILENO, msg, strlen(msg));
}
