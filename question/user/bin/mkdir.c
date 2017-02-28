#include "prog.h"

void main_prog_mkdir(char* path) {

    if (mkdir(path) < 0) {
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
