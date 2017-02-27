#include "prog_cat.h"

void main_prog_cat_read(int x1, int x2, int x3, int x4) {
    int fd = open("/file.txt", READ);

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

void main_prog_cat_write() {
    int fd = open("/file.txt", WRITE);

    if (fd < 0) {
        err("Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    char text[1024] = "123 testing blah\n";

    if (write(fd, text, sizeof(text)) < 0) {
        err("Failed to write to file\n");
        exit(EXIT_FAILURE);
    }

    if (close(fd) < 0) {
        err("Failed to close file\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
