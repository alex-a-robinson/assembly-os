#include "console.h"

void gets(char* x, int n) {
    for (int i = 0; i < n; i++) {
        x[i] = PL011_getc(UART1, true); // UART1?

        if (x[i] == '\x0A') {
            x[i] = '\x00';
            break;
        }
    }
}

void _err(char* msg) {
    PL011_t* device = UART1;
    int n = strlen(msg);
    for (int i = 0; i < n; i++) {
        PL011_putc(device, *msg++, true);
    }
}

/* Since we lack a *real* loader (as a result of lacking a storage
* medium to store program images), the following approximates one:
* given a program name, from the set of programs statically linked
* into the kernel image, it returns a pointer to the entry point.
*/

extern void main_P3();
extern void main_P4();
extern void main_P5();
extern void main_dp();
extern void main_TEST1();
extern void main_prog_cat_read();
extern void main_prog_cat_write();



void* load(char* x) {
    if (0 == strcmp(x, "P3")) {
        return &main_P3;
    } else if (0 == strcmp(x, "P4")) {
        return &main_P4;
    } else if (0 == strcmp(x, "P5")) {
        return &main_P5;
    } else if (0 == strcmp(x, "DP")) {
        return &main_dp;
    } else if (0 == strcmp(x, "TEST1")) {
        return &main_TEST1;
    } else if (0 == strcmp(x, "catw")) {
        return &main_prog_cat_write;
    } else if (0 == strcmp(x, "catr")) {
        return &main_prog_cat_read;
    }

    return NULL;
}

/* The behaviour of the console process can be summarised as an
* (infinite) loop over three main steps, namely
*
* 1. write a command prompt then read a command,
* 2. split the command into space-separated tokens using strtok,
* 3. execute whatever steps the command dictates.
*/

void cmd_ls(char* path) {
    char file_list[MAX_PATH_LENGTH];
    memset(file_list, 0, MAX_PATH_LENGTH);
    if (ls(path, file_list) < 0) {
        return;
    }
    _err(file_list);
}

void cmd_stat(char* path) {
    file_stat_t file_info;
    memset(&file_info, 0, sizeof(file_stat_t));
    if (stat(path, &file_info) < 0) {
        return;
    }

    // TODO not priting the info
    char b[1024];
    _err("TYPE "); _err(ss(b, file_info.type)); _err("\n");
    _err("TYPE "); _err(ss(b, file_info.size)); _err("\n");
    _err("TYPE "); _err(ss(b, file_info.creation_time)); _err("\n");
    _err("TYPE "); _err(ss(b, file_info.modification_time)); _err("\n");
}

void cmd_cd(char* cwd, char* path) {
    char cwd_copy[MAX_PATH_LENGTH];
    strcpy(cwd_copy, cwd);
    if (path[0] == '/') { // Relative to root
        strcpy(cwd_copy, path);
    } else { // Otherwise append
        strcat(cwd_copy, path);
    }

    // Append trailing slash
    if (cwd_copy[strlen(cwd_copy)-1] != '/') {
        strcat(cwd_copy, "/");
    }

    // Check type of result
    file_stat_t file_info;
    memset(&file_info, 0, sizeof(file_stat_t));
    if (stat(cwd_copy, &file_info) < 0) {
        _err("Failed to open "); _err(cwd_copy); _err("\n");
        return;
    }
    if (file_info.type != INODE_DIRECTORY) {
        _err("This is not a directory\n");
        return;
    }

    // Finally copy into cwd
    strcpy(cwd, cwd_copy);
}

void main_console() {
    // Initilise the current working directory
    char cwd[MAX_PATH_LENGTH];
    memset(cwd, 0, MAX_PATH_LENGTH);
    strcpy(cwd, "/");

    mount();

    char* p, x[1024];

    while (1) {
        //write("shell$ ", 7);
        write(STDERR_FILENO, "shell$ ", 7);
        gets(x, 1024);
        //read(STDIN_FILENO, x, 1024);
        p = strtok(x, " ");

        if (0 == strcmp(p, "fork")) {
            pid_t pid = fork();

            if (0 == pid) {
                void* addr = load(strtok(NULL, " "));
                exec(addr);
            }
        } else if (0 == strcmp(p, "kill")) {
            pid_t pid = atoi(strtok(NULL, " "));
            int s = atoi(strtok(NULL, " "));

            kill(pid, s);
        } else if (0 == strcmp(p, "ps")) {
            pid_t pid = atoi(strtok(NULL, " "));
            ps(pid);
        } else if (0 == strcmp(p, "ls")) {
            char* path = strtok(NULL, " ");
            if (path == NULL) {
                cmd_ls(cwd);
            } else {
                cmd_ls(path);
            }
        }  else if (0 == strcmp(p, "stat")) {
            char* path = strtok(NULL, " ");
            if (path == NULL) {
                cmd_stat(cwd);
            } else {
                cmd_stat(path);
            }
        }  else if (0 == strcmp(p, "cd")) {
            char* path = strtok(NULL, " ");
            if (path == NULL) {
                cmd_cd(cwd, "/");
            } else {
                cmd_cd(cwd, path);
            }
        } else if (p == NULL) {
            continue; // Continue if no input
        } else {
            err("unknown command\n");
        }
    }

    exit(EXIT_SUCCESS);
}
