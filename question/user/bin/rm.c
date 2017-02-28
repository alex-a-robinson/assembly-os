#include "prog.h"

void main_prog_rm(char* args) {

    char path[MAX_PATH_LENGTH];
    path_from_args(CWD, args, path);

    if (rm(path) < 0) {
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
