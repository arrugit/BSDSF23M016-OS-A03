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
#include <termios.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "
#define MAX_PIPE_CMDS 5
#define MAX_JOBS 50

typedef struct {
    pid_t pid;
    pid_t pgid;
    char command[256];
    int active;
} Job;

/* global/state */
extern Job jobs[MAX_JOBS];
extern int job_count;
extern pid_t foreground_pid;   /* pid of last foreground process or -1 */
extern pid_t shell_pgid;       /* shell process group id */
extern struct termios shell_tmodes;

/* Function prototypes */
char* read_cmd(char* prompt);
char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char** arglist);
int execute_with_redirection(char** arglist, int background);
int execute_pipeline(char*** cmdlist, int cmdcount, int background);
char*** parse_pipeline(char* cmdline, int* cmdcount);

void check_background_jobs();
void add_job(pid_t pid, pid_t pgid, char* cmd);
void list_jobs();
void init_shell();                 /* initialize shell process group + terminal */
void setup_signal_handlers();      /* optional: other signal handlers */

#endif // SHELL_H
