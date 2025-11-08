#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "
#define MAX_PIPE_CMDS 5
#define MAX_JOBS 50

// Structure for background jobs
typedef struct {
    pid_t pid;
    char command[256];
    int active;
} Job;

// Function prototypes
char* read_cmd(char* prompt);
char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char** arglist);
int execute_with_redirection(char** arglist, int background);
int execute_pipeline(char*** cmdlist, int cmdcount, int background);
char*** parse_pipeline(char* cmdline, int* cmdcount);
void check_background_jobs();
void add_job(pid_t pid, char* cmd);
void list_jobs();

#endif // SHELL_H
