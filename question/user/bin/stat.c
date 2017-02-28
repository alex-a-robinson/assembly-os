#include "prog.h"

void main_prog_stat(char* args) {
    char path[100];
    path_from_args(CWD, args, path);

    file_stat_t file_info;
    memset(&file_info, 0, sizeof(file_stat_t));
    if (stat(path, &file_info) < 0) {
        exit(EXIT_FAILURE);
    }

    // TODO not priting the info
    char b[1024];
    puts("Type: "); puts(ss(b, file_info.type)); puts("\n");
    puts("Size (bytes): "); puts(ss(b, file_info.size)); puts("\n");
    puts("Creation Time: "); puts(ss(b, file_info.creation_time)); puts("\n");
    puts("Modification Time "); puts(ss(b, file_info.modification_time)); puts("\n");

    exit(EXIT_SUCCESS);
}
