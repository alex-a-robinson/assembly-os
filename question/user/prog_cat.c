#include "prog_cat.h"

void main_prog_cat(char* args) {
    int fd = open(args, READ);

    if (fd < 0) {
        err("Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    char text[1024];

    if (read(fd, text, sizeof(text)) < 0) {
        err("Failed to read to file\n");
        exit(EXIT_FAILURE);
    }

    if (close(fd) < 0) {
        err("Failed to close file\n");
        exit(EXIT_FAILURE);
    }

    err(text);

    exit(EXIT_SUCCESS);
}
