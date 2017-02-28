#include "prog.h"

void main_prog_wc(char* args) {
    char path[1024];
    char text[1024];

    path_from_args(CWD, args, path);

    int fd = open(path, READ);

    if (fd < 0) {
        err("Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    file_stat_t file_info;
    if (stat(path, &file_info)) {
        err("Failed to get file info\n");
        exit(EXIT_FAILURE);
    }

    if (read(fd, text, file_info.size) < 0) {
        err("Failed to write to file\n");
        exit(EXIT_FAILURE);
    }

    if (close(fd) < 0) {
        err("Failed to close file\n");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    char* a = strtok(text, "\n");
    while(a != NULL) {
        a = strtok(NULL, "\n");
        count++;
    }

    // Convert to a string
    char b[10];
    ss(b, count);

    puts(b);

    exit(EXIT_SUCCESS);
}
