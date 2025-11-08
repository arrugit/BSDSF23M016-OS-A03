#include "shell.h"

/* -----------------------------------------------------
   Execute with redirection (<, >, >>)
   ----------------------------------------------------- */
int execute_with_redirection(char** arglist) {
    int in = -1, out = -1;
    int append = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], ">") == 0 || strcmp(arglist[i], ">>") == 0) {
            append = (strcmp(arglist[i], ">>") == 0);
            out = open(arglist[i + 1],
                       O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC),
                       0644);
            if (out < 0) {
                perror("open output file");
                return -1;
            }
            arglist[i] = NULL;
            break;
        } else if (strcmp(arglist[i], "<") == 0) {
            in = open(arglist[i + 1], O_RDONLY);
            if (in < 0) {
                perror("open input file");
                return -1;
            }
            arglist[i] = NULL;
            break;
        }
    }

    int pid = fork();
    if (pid == 0) {
        if (in != -1) dup2(in, STDIN_FILENO);
        if (out != -1) dup2(out, STDOUT_FILENO);
        execvp(arglist[0], arglist);
        perror("exec failed");
        exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
        if (in != -1) close(in);
        if (out != -1) close(out);
    } else {
        perror("fork failed");
    }

    return 0;
}

/* -----------------------------------------------------
   Execute pipeline (cmd1 | cmd2 | cmd3 ...)
   ----------------------------------------------------- */
int execute_pipeline(char*** cmdlist, int cmdcount) {
    int pipefd[2], in_fd = 0;

    for (int i = 0; i < cmdcount; i++) {
        pipe(pipefd);
        pid_t pid = fork();

        if (pid == 0) {
            dup2(in_fd, STDIN_FILENO);
            if (i < cmdcount - 1)
                dup2(pipefd[1], STDOUT_FILENO);

            close(pipefd[0]);
            execvp(cmdlist[i][0], cmdlist[i]);
            perror("exec failed");
            exit(1);
        } else {
            wait(NULL);
            close(pipefd[1]);
            in_fd = pipefd[0];
        }
    }

    return 0;
}
