#include "shell.h"

extern Job jobs[MAX_JOBS];
extern int job_count;
extern pid_t foreground_pid;
extern pid_t shell_pgid;

/* ---------------- execute single command with redirection ---------------- */
int execute_with_redirection(char** arglist, int background) {
    int in = -1, out = -1, append = 0;

    /* handle redirection tokens; modify arglist in place */
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
    if (pid < 0) { perror("fork"); return -1; }

    if (pid == 0) {
        /* Child */
        /* create new process group for job */
        setpgid(0, 0); /* child becomes leader of new pgid = pid */

        /* if foreground, take terminal */
        if (!background) tcsetpgrp(STDIN_FILENO, getpid());

        /* restore default signals */
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        if (in != -1) dup2(in, STDIN_FILENO);
        if (out != -1) dup2(out, STDOUT_FILENO);

        execvp(arglist[0], arglist);
        perror("exec failed");
        _exit(127);
    } else {
        /* Parent */
        /* ensure child pgid set (use pid as pgid) */
        setpgid(pid, pid);

        if (background) {
            add_job(pid, pid, arglist[0], JOB_RUNNING);
        } else {
            /* foreground: give terminal to child's pgid */
            tcsetpgrp(STDIN_FILENO, pid);
            foreground_pid = pid;

            int status;
            waitpid(pid, &status, WUNTRACED);
            if (WIFSTOPPED(status)) {
                /* child stopped: add job as stopped */
                add_job(pid, pid, arglist[0], JOB_STOPPED);
                printf("\n[+] Process %d stopped\n", (int)pid);
            } else {
                /* exited or signaled: do nothing (reaped) */
            }

            foreground_pid = -1;
            /* restore terminal to shell */
            tcsetpgrp(STDIN_FILENO, shell_pgid);
        }

        if (in != -1) close(in);
        if (out != -1) close(out);
    }
    return 0;
}

/* ---------------- execute pipeline ---------------- */
int execute_pipeline(char*** cmdlist, int cmdcount, int background) {
    int in_fd = -1;
    int pipefd[2];
    pid_t pids[MAX_PIPE_CMDS];
    pid_t pgid = 0;

    for (int i = 0; i < cmdcount; i++) {
        if (i < cmdcount - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe failed");
                return -1;
            }
        } else {
            pipefd[0] = -1; pipefd[1] = -1;
        }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return -1; }

        if (pid == 0) {
            /* Child */
            /* first child becomes pgid leader */
            if (pgid == 0) pgid = getpid();
            setpgid(0, pgid);

            /* if foreground, give terminal to pipeline pgid */
            if (!background) tcsetpgrp(STDIN_FILENO, pgid);

            /* restore default signal handlers */
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);

            /* input from previous command */
            if (in_fd != -1) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            /* output to next command if not last */
            if (i < cmdcount - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
                close(pipefd[0]);
            }

            execvp(cmdlist[i][0], cmdlist[i]);
            perror("exec failed");
            _exit(127);
        } else {
            /* Parent */
            if (pgid == 0) pgid = pid;
            setpgid(pid, pgid);
            pids[i] = pid;

            if (in_fd != -1) close(in_fd);
            if (i < cmdcount - 1) {
                close(pipefd[1]);
                in_fd = pipefd[0];
            } else {
                in_fd = -1;
            }
        }
    }

    /* record or wait on pipeline */
    if (background) {
        /* add job using last pid & pgid */
        add_job(pids[cmdcount - 1], pgid, cmdlist[0][0], JOB_RUNNING);
    } else {
        /* foreground pipeline: give terminal and wait for all */
        tcsetpgrp(STDIN_FILENO, pgid);
        foreground_pid = pids[cmdcount - 1];

        for (int i = 0; i < cmdcount; i++) {
            int status;
            pid_t w = waitpid(pids[i], &status, WUNTRACED);
            if (w == -1) {
                if (errno == ECHILD) continue;
                if (errno == EINTR) { i--; continue; }
                perror("waitpid");
                break;
            }
            if (WIFSTOPPED(status)) {
                /* add pipeline as stopped job */
                add_job(pids[cmdcount - 1], pgid, cmdlist[0][0], JOB_STOPPED);
                printf("\n[+] Pipeline stopped (pgid=%d)\n", (int)pgid);
                break;
            }
        }

        foreground_pid = -1;
        tcsetpgrp(STDIN_FILENO, shell_pgid);
    }

    return 0;
}
