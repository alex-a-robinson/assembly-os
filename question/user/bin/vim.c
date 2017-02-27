#include "vim.h"

void main_prog_vim(char* args) {
    char filename[MAX_FILE_NAME_LENGTH];
    char text[1024];
    memset(text, 0, 1024);

    if (parse_cmd(args, filename, text) < 0) {
        err("Incorrect args, vim <filename> <text>\n");
        exit(EXIT_FAILURE);
    }

    // If no text get input from the user
    if (strlen(text) == 0) {
        read(STDIN_FILENO, text, 1024);
    }

    char path[MAX_PATH_LENGTH];
    path_from_args(CWD, filename, path);

    int fd = open(path, WRITE);

    if (fd < 0) {
        err("Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    if (write(fd, text, strlen(text)) < 0) {
        err("Failed to write to file\n");
        exit(EXIT_FAILURE);
    }

    if (close(fd) < 0) {
        err("Failed to close file\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
