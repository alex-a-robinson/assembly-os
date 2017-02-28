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
extern void main_prog_vima();
extern void main_prog_wc();
extern void main_prog_echo();
extern void main_prog_cd();
extern void main_prog_kill();
extern void main_prog_ls();
extern void main_prog_ps();
extern void main_prog_stat();

prog_t progs[] = {
    {main_P3, "P3"},
    {main_P4, "P4"},
    {main_P5, "P5"},
    {main_dp, "DP"},
    {main_TEST1, "test1"},
    {main_prog_cat, "cat"},
    {main_prog_vim, "vim"},
    {main_prog_vima, "vima"},
    {main_prog_wc, "wc"},
    {main_prog_echo, "echo"},
    {main_prog_cd, "cd"},
    {main_prog_kill, "kill"},
    {main_prog_ls, "ls"},
    {main_prog_ps, "ps"},
    {main_prog_stat, "stat"}
};

void* load(char* x) {
    for (int i=0; i < sizeof(progs)/sizeof(prog_t); i++) {
        if (strcmp(progs[i].name, x) == 0) {
            return progs[i].addr;
        }
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

void cmd_fork(char* args, int wait) {
    pid_t pid;
    if (wait) {
        pid = fork();
    } else {
        pid = fork_wait();
    }

    if (0 == pid) {
        char prog_args[100];
        memset(prog_args, 0, 100);
        char prog[100];
        parse_cmd(args, prog, prog_args);
        void* addr = load(prog);
        exec(addr, prog_args);
    }

    if (wait) {
        waitp(pid);
    }
}

int handle_cmd(char* input, char* cmd, char* args) {
    if (strcmp(cmd, "fork") == 0) {
        cmd_fork(args, 0);
    } else if (strcmp(cmd, "shutdown") == 0) { // Exit
        return -1;
    } else if (load(cmd) != NULL) {
        cmd_fork(input, 1);
    } else {
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
        if (handle_cmd(input, cmd, args) < 0) {
            break;
        }
    }

    err("System going down, bye!\n");
    unmount();

    exit(EXIT_SUCCESS);
}
