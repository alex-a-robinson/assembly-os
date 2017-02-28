#include "prog.h"

void main_prog_vima(char* args) {
    char filename[MAX_FILE_NAME_LENGTH];
    char text_read[1024];
    char text_to_write[1024];
    memset(text_read, 0, 1024);
    memset(text_to_write, 0, 1024);

    if (parse_cmd(args, filename, text_read) < 0) {
        err("Incorrect args, vima <filename> <text>\n");
        exit(EXIT_FAILURE);
    }

    char path[MAX_PATH_LENGTH];
    path_from_args(CWD, filename, path);

    int fd = open(path, READ_WRITE);

    if (fd < 0) {
        err("Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    if (read(fd, text_to_write, sizeof(text_to_write)) < 0) {
        err("Failed to read to file\n");
        exit(EXIT_FAILURE);
    }

    puts(text_to_write);

    // If no text get input from the user
    if (strlen(text_read) == 0) {
        read(STDIN_FILENO, text_read, 1024);
    }

    strcat(text_to_write, text_read);

    if (write(fd, text_to_write, strlen(text_to_write)+1) < 0) {
        err("Failed to write to file\n");
        exit(EXIT_FAILURE);
    }

    if (close(fd) < 0) {
        err("Failed to close file\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void main_prog_vim(char* args) {
    char filename[MAX_FILE_NAME_LENGTH];
    char text[1024];
    memset(text, 0, 1024);

    if (parse_cmd(args, filename, text) < 0) {
        err("Incorrect args, vim <filename> <text>\n");
        exit(EXIT_FAILURE);
    }

    char path[MAX_PATH_LENGTH];
    path_from_args(CWD, filename, path);

    int fd = open(path, WRITE);

    if (fd < 0) {
        err("Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    // If no text get input from the user
    if (strlen(text) == 0) {
        read(STDIN_FILENO, text, 1024);
    }

    if (write(fd, text, strlen(text)+1) < 0) {
        err("Failed to write to file\n");
        exit(EXIT_FAILURE);
    }

    if (close(fd) < 0) {
        err("Failed to close file\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
