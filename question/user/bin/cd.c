#include "prog.h"

void main_prog_cd(char* path) {

    // No change
    if (strcmp(path, ".") == 0) {
        exit(EXIT_SUCCESS);
    }

    // If no args, go to root
    if (strlen(path) == 0) {
        strcpy(CWD, "/");
        exit(EXIT_FAILURE);
    }

    char CWD_copy[MAX_PATH_LENGTH];
    path_from_args(CWD, path, CWD_copy);

    // Append trailing slash
    if (CWD_copy[strlen(CWD_copy)-1] != '/') {
        strcat(CWD_copy, "/");
    }

    // Check type of result
    file_stat_t file_info;
    memset(&file_info, 0, sizeof(file_stat_t));
    if (stat(CWD_copy, &file_info) < 0) {
        err("Failed to open "); err(CWD_copy); err("\n");
        exit(EXIT_FAILURE);
    }
    if (file_info.type != INODE_DIRECTORY) {
        err("This is not a directory\n");
        exit(EXIT_FAILURE);
    }

    // Finally copy into CWD
    strcpy(CWD, CWD_copy);
    exit(EXIT_SUCCESS);
}
