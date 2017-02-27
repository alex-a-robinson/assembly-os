#include "console.h"

extern char CWD[MAX_PATH_LENGTH];

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
extern void main_prog_cat();
extern void main_prog_vim();



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
    } else if (0 == strcmp(x, "cat")) {
        return &main_prog_cat;
    } else if (0 == strcmp(x, "vim")) {
        return &main_prog_vim;
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

void cmd_fork(char* args) {
    pid_t pid = fork();

    if (0 == pid) {
        char prog_args[100];
        memset(prog_args, 0, 100);
        char prog[100];
        parse_cmd(args, prog, prog_args);
        void* addr = load(prog);
        exec(addr, prog_args);
    }
}
void cmd_kill(char* args) {
    pid_t pid = atoi(strtok(args, " "));
    int s = atoi(strtok(NULL, " "));

    kill(pid, s);
}
void cmd_ps(char* args) {
    pid_t pid = atoi(strtok(args, " "));
    ps(pid);
}

void cmd_ls(char* args) {
    char path[100];
    path_from_args(CWD, args, path);

    char file_list[MAX_PATH_LENGTH];
    memset(file_list, 0, MAX_PATH_LENGTH);
    if (ls(path, file_list) < 0) {
        return;
    }
    _err(file_list);
}

void cmd_stat(char* args) {
    char path[100];
    path_from_args(CWD, args, path);

    file_stat_t file_info;
    memset(&file_info, 0, sizeof(file_stat_t));
    if (stat(path, &file_info) < 0) {
        return;
    }

    // TODO not priting the info
    char b[1024];
    _err("Type: "); _err(ss(b, file_info.type)); _err("\n");
    _err("Size (bytes): "); _err(ss(b, file_info.size)); _err("\n");
    _err("Creation Time: "); _err(ss(b, file_info.creation_time)); _err("\n");
    _err("Modification Time "); _err(ss(b, file_info.modification_time)); _err("\n");
}

void cmd_cd(char* path) {
    // If no args, go to root
    if (strlen(path) == 0) {
        strcpy(CWD, "/");
        return;
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
        _err("Failed to open "); _err(CWD_copy); _err("\n");
        return;
    }
    if (file_info.type != INODE_DIRECTORY) {
        _err("This is not a directory\n");
        return;
    }

    // Finally copy into CWD
    strcpy(CWD, CWD_copy);
}

// NOTE simplification over real system which alters the file descriptors
int cmd_write(char* args) {
    char filename[MAX_FILE_NAME_LENGTH];
    char text[1024];
    memset(text, 0, 1024);

    if (parse_cmd(args, filename, text) < 0) {
        err("Incorrect args, write <filename> <text>\n");
        return -1;
    }

    char path[MAX_PATH_LENGTH];
    path_from_args(CWD, filename, path);

    int fd = open(path, WRITE);
    if (fd < 0) {
        err("Failed to open file\n");
        return -1;
    }

    if (write(fd, text, strlen(text)) < 0) {
        err("Failed to write to file\n");
        return -1;
    }

    if (close(fd) < 0) {
        err("Failed to close file\n");
        return -1;
    }
    return 0;
}

void cmd_echo(char* args) {
    write(STDOUT_FILENO, args, strlen(args));
}

int handle_cmd(char* cmd, char* args) {
    if (strcmp(cmd, "fork") == 0) {
        cmd_fork(args);
    } else if (strcmp(cmd, "kill") == 0) {
        cmd_kill(args);
    } else if (strcmp(cmd, "ps") == 0) {
        cmd_ps(args);
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls(args);
    }  else if (strcmp(cmd, "stat") == 0) {
        cmd_stat(args);
    }  else if (strcmp(cmd, "cd") == 0) {
        cmd_cd(args);
    } else if (strcmp(cmd, "shutdown") == 0) { // Exit
        return -1;
    } else if (strcmp(cmd, "echo") == 0) {
        cmd_echo(args);
    } else if (strcmp(cmd, "write") == 0) {
        cmd_write(args);
    }else {
        err("unknown command\n");
    }

    return 0;
}

void main_console() {
    // Initilise the current working directory
    strcpy(CWD, "/");

    mount();

    char cmd[100];
    char args[1024];
    char input[1024];
    char prompt[100];

    while (1) {
        memset(cmd, 0, 100);
        memset(args, 0, 1024);
        memset(input, 0, 1024);
        memset(prompt, 0, 100);

        // Set the prompt
        strcpy(prompt, CWD);
        strcat(prompt, " $ ");

        // Write the prompt
        write(STDERR_FILENO, prompt, strlen(prompt));
        gets(input, 1024);

        if (parse_cmd(input, cmd, args) < 0) { // Continue if nothing entered
            continue;
        }
        if (handle_cmd(cmd, args) < 0) {
            break;
        }
    }

    err("System going down, bye!\n");
    unmount();

    exit(EXIT_SUCCESS);
}
