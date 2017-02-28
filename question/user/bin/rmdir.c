#include "prog.h"

void main_prog_rmdir(char* path) {

    if (rmdir(path) < 0) {
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
