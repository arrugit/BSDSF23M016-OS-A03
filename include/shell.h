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
#define ARGLEN 256
#define PROMPT "FCIT> "
#define MAX_PIPE_CMDS 10
#define MAX_JOBS 128

typedef enum { JOB_RUNNING = 0, JOB_STOPPED = 1, JOB_DONE = 2 } JobStatus;

typedef struct {
    pid_t pid;           /* last pid in the job (useful for reporting) */
    pid_t pgid;          /* process group id for the job */
    char command[512];   /* textual command */
    JobStatus status;    /* running / stopped / done */
    int active;          /* 1 = active (running or stopped), 0 = removed/done */
} Job;

/* Global state (defined in main.c) */
extern Job jobs[MAX_JOBS];
extern int job_count;
extern pid_t foreground_pid;   /* pid of last foreground process (for compatibility) */
extern pid_t shell_pgid;       /* shell pgid */
extern struct termios shell_tmodes;

/* I/O and parsing */
char* read_cmd(char* prompt);
char** tokenize(char* cmdline);
char*** parse_pipeline(char* cmdline, int* cmdcount);

/* Execution */
int execute_with_redirection(char** arglist, int background);
int execute_pipeline(char*** cmdlist, int cmdcount, int background);

/* Builtins / job control */
int handle_builtin(char** arglist);
void add_job(pid_t pid, pid_t pgid, const char* cmd, JobStatus status);
void check_background_jobs();
void list_jobs();
int job_index_from_token(const char* tok); /* helper to parse %n or number */
int find_job_by_index(int idx); /* index is 1-based */
int fg_builtin(int job_index);
int bg_builtin(int job_index);

/* Initialization */
void init_shell();
void setup_signal_handlers();

#endif // SHELL_H
