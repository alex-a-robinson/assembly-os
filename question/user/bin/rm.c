#include "prog.h"

void main_prog_rm(char* path) {

    if (rm(path) < 0) {
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
