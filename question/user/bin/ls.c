#include "prog.h"

void main_prog_ls(char* args) {
    char path[100];
    path_from_args(CWD, args, path);

    char file_list[MAX_PATH_LENGTH];
    memset(file_list, 0, MAX_PATH_LENGTH);
    if (ls(path, file_list) < 0) {
        exit(EXIT_FAILURE);
    }
    puts(file_list);
    exit(EXIT_SUCCESS);
}
