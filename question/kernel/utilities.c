#include "utilities.h"

void error(char* msg) {
    sys_write(STDERR_FILENO, msg, strlen(msg));
}
