#include "shell.h"

extern Job jobs[MAX_JOBS];
extern int job_count;

/* -----------------------------------------------------
   Read command line using GNU Readline
   ----------------------------------------------------- */
char* read_cmd(char* prompt) {
    char* input = readline(prompt);
    if (input == NULL)
        return NULL;
    if (strlen(input) > 0)
        add_history(input);
    return input;
}

/* -----------------------------------------------------
   Tokenize a single command (space-delimited)
   ----------------------------------------------------- */
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0')
        return NULL;

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    if (!arglist) {
        perror("malloc failed");
        exit(1);
    }

    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        if (!arglist[i]) {
            perror("malloc failed");
            exit(1);
        }
        memset(arglist[i], 0, ARGLEN);
    }

    char* token = strtok(cmdline, " \t");
    int argnum = 0;

    while (token != NULL && argnum < MAXARGS) {
        strncpy(arglist[argnum], token, ARGLEN - 1);
        argnum++;
        token = strtok(NULL, " \t");
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

/* -----------------------------------------------------
   Built-in command handler
   ----------------------------------------------------- */
int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        char *dir = arglist[1];
        if (dir == NULL) dir = getenv("HOME");
        if (chdir(dir) != 0)
            perror("cd");
        return 1;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>     Change directory\n");
        printf("  exit         Exit shell\n");
        printf("  help         Show help message\n");
        printf("  history      Show command history\n");
        printf("  jobs         Show background jobs\n");
        printf("Supports redirection (<, >, >>), pipes (|), and background (&)\n");
        return 1;
    }

    if (strcmp(arglist[0], "history") == 0) {
        HIST_ENTRY **hist = history_list();
        if (hist)
            for (int i = 0; hist[i]; i++)
                printf("%d  %s\n", i + 1, hist[i]->line);
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        list_jobs();
        return 1;
    }

    return 0;
}

/* -----------------------------------------------------
   Parse a pipeline (split commands separated by |)
   ----------------------------------------------------- */
char*** parse_pipeline(char* cmdline, int* cmdcount) {
    char* cmds[MAX_PIPE_CMDS];
    int count = 0;

    char* token = strtok(cmdline, "|");
    while (token != NULL && count < MAX_PIPE_CMDS) {
        cmds[count++] = strdup(token);
        token = strtok(NULL, "|");
    }
    *cmdcount = count;

    char*** cmdlist = malloc(count * sizeof(char**));
    for (int i = 0; i < count; i++) {
        cmdlist[i] = tokenize(cmds[i]);
        free(cmds[i]);
    }

    return cmdlist;
}

/* -----------------------------------------------------
   Job management functions
   ----------------------------------------------------- */
void add_job(pid_t pid, char* cmd) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].command, cmd, sizeof(jobs[job_count].command) - 1);
        jobs[job_count].active = 1;
        printf("[%d] %d  %s\n", job_count + 1, pid, cmd);
        job_count++;
    }
}

void check_background_jobs() {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].active) {
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
            if (result != 0) {
                jobs[i].active = 0;
                printf("\n[%d] Done  %s\n", i + 1, jobs[i].command);
            }
        }
    }
}

void list_jobs() {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].active)
            printf("[%d] Running  %s\n", i + 1, jobs[i].command);
    }
}
