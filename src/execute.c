#include "shell.h"

extern Job jobs[MAX_JOBS];
extern int job_count;
extern pid_t foreground_pid;
extern pid_t shell_pgid;

/* -----------------------------------------------------
   Execute single command (redirection + background)
   ----------------------------------------------------- */
int execute_with_redirection(char** arglist, int background) {
    int in = -1, out = -1, append = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], ">") == 0 || strcmp(arglist[i], ">>") == 0) {
            append = (strcmp(arglist[i], ">>") == 0);
            out = open(arglist[i + 1],
                       O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC),
                       0644);
            if (out < 0) { perror("open output"); return -1; }
            arglist[i] = NULL;
            break;
        } else if (strcmp(arglist[i], "<") == 0) {
            in = open(arglist[i + 1], O_RDONLY);
            if (in < 0) { perror("open input"); return -1; }
            arglist[i] = NULL;
            break;
        }
    }

    pid_t pid = fork();

    if (pid == 0) {
        /* Child */
        /* Put child in its own process group (job leader) */
        setpgid(0, 0);

        /* If this is a foreground job, give it the terminal */
        if (!background) {
            tcsetpgrp(STDIN_FILENO, getpid());
        }

        /* Restore default handlers so child responds to Ctrl+C/Ctrl+Z */
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        if (in  != -1) dup2(in,  STDIN_FILENO);
        if (out != -1) dup2(out, STDOUT_FILENO);

        execvp(arglist[0], arglist);
        perror("exec failed");
        _exit(1);
    } else if (pid > 0) {
        /* Parent */
        /* ensure child is in its own pgid */
        setpgid(pid, pid);

        if (background) {
            add_job(pid, pid, arglist[0]);
        } else {
            /* give terminal control to child's pgid while waiting */
            tcsetpgrp(STDIN_FILENO, pid);
            foreground_pid = pid;
            int status;
            waitpid(pid, &status, WUNTRACED);
            if (WIFSTOPPED(status)) {
                /* child stopped: add to jobs as backgrounded/stopped job */
                add_job(pid, pid, arglist[0]);
                printf("\n[+] Process %d stopped\n", pid);
            }
            foreground_pid = -1;
            /* restore terminal to shell */
            tcsetpgrp(STDIN_FILENO, shell_pgid);
        }

        if (in  != -1) close(in);
        if (out != -1) close(out);
    } else {
        perror("fork failed");
    }
    return 0;
}

/* -----------------------------------------------------
   Execute pipeline
   ----------------------------------------------------- */
int execute_pipeline(char*** cmdlist, int cmdcount, int background) {
    int pipefd[2], in_fd = -1;
    pid_t pids[MAX_PIPE_CMDS];
    pid_t pgid = 0; /* process group id for the pipeline */

    for (int i = 0; i < cmdcount; i++) {
        if (i < cmdcount - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe failed");
                return -1;
            }
        } else {
            /* last command: no pipe created */
            pipefd[0] = -1;
            pipefd[1] = -1;
        }

        pid_t pid = fork();

        if (pid == 0) {
            /* Child */
            /* set pgid for pipeline: first child becomes pgid leader */
            if (pgid == 0) pgid = getpid();
            setpgid(0, pgid);

            /* If foreground pipeline, give it terminal */
            if (!background) tcsetpgrp(STDIN_FILENO, pgid);

            /* Restore default signals in children */
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);

            /* set up input from previous pipe */
            if (in_fd != -1) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            /* if not last, set stdout to write end */
            if (i < cmdcount - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
                close(pipefd[0]);
            }

            execvp(cmdlist[i][0], cmdlist[i]);
            perror("exec failed");
            _exit(1);
        } else if (pid > 0) {
            /* Parent */
            if (pgid == 0) pgid = pid;
            setpgid(pid, pgid);
            pids[i] = pid;

            /* close unused ends in parent */
            if (in_fd != -1) close(in_fd);
            if (i < cmdcount - 1) {
                close(pipefd[1]);
                in_fd = pipefd[0]; /* next command reads from here */
            } else {
                in_fd = -1;
            }
        } else {
            perror("fork failed");
            return -1;
        }
    }

    /* After forking pipeline children */
    if (background) {
        /* record pipeline last pid & pgid (use pgid) */
        add_job(pids[cmdcount - 1], pgid, cmdlist[0][0]);
    } else {
        /* Foreground: give terminal to pipeline pgid and wait for all */
        tcsetpgrp(STDIN_FILENO, pgid);
        foreground_pid = pids[cmdcount - 1];
        for (int i = 0; i < cmdcount; i++) {
            int status;
            waitpid(pids[i], &status, WUNTRACED);
            if (WIFSTOPPED(status)) {
                /* if any member stopped, add job for pgid */
                add_job(pids[cmdcount - 1], pgid, cmdlist[0][0]);
                printf("\n[+] Pipeline stopped (pgid=%d)\n", pgid);
                break;
            }
        }
        foreground_pid = -1;
        tcsetpgrp(STDIN_FILENO, shell_pgid);
    }

    return 0;
}
