#include "shell.h"

extern Job jobs[MAX_JOBS];
extern int job_count;
extern pid_t foreground_pid;

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
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        setpgid(0, 0);               /* own process group */

        if (in  != -1) dup2(in,  STDIN_FILENO);
        if (out != -1) dup2(out, STDOUT_FILENO);

        execvp(arglist[0], arglist);
        perror("exec failed");
        _exit(1);
    } else if (pid > 0) {
        /* Parent */
        setpgid(pid, pid);           /* isolate child */
        if (background) {
            add_job(pid, arglist[0]);
        } else {
            foreground_pid = pid;
            waitpid(pid, NULL, 0);
            foreground_pid = -1;
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
    int pipefd[2], in_fd = 0;
    pid_t pids[MAX_PIPE_CMDS];

    for (int i = 0; i < cmdcount; i++) {
        pipe(pipefd);
        pid_t pid = fork();

        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            setpgid(0, 0);

            dup2(in_fd, STDIN_FILENO);
            if (i < cmdcount - 1)
                dup2(pipefd[1], STDOUT_FILENO);

            close(pipefd[0]);
            execvp(cmdlist[i][0], cmdlist[i]);
            perror("exec failed");
            _exit(1);
        } else if (pid > 0) {
            setpgid(pid, pids[0] ? pids[0] : pid);
            pids[i] = pid;
            close(pipefd[1]);
            in_fd = pipefd[0];
        } else {
            perror("fork failed");
            return -1;
        }
    }

    if (background) {
        add_job(pids[cmdcount - 1], cmdlist[0][0]);
    } else {
        foreground_pid = pids[cmdcount - 1];
        for (int i = 0; i < cmdcount; i++)
            waitpid(pids[i], NULL, 0);
        foreground_pid = -1;
    }
    return 0;
}
