#include "shell.h"

/* -----------------------------------------------------
   Read command line using GNU Readline
   ----------------------------------------------------- */
char* read_cmd(char* prompt) {
    char* input = readline(prompt);
    if (input == NULL) // Ctrl+D
        return NULL;
    if (strlen(input) > 0)
        add_history(input);
    return input;
}

/* -----------------------------------------------------
   Tokenize input string into arguments
   ----------------------------------------------------- */
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0')
        return NULL;

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    if (arglist == NULL) {
        perror("malloc failed");
        exit(1);
    }

    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        if (arglist[i] == NULL) {
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
        for (int i = 0; i < MAXARGS + 1; i++)
            free(arglist[i]);
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

    // exit
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    // cd
    if (strcmp(arglist[0], "cd") == 0) {
        char *dir = arglist[1];
        if (dir == NULL) {
            dir = getenv("HOME");
            if (dir == NULL)
                dir = "/";
        }
        if (chdir(dir) != 0)
            perror("cd");
        return 1;
    }

    // help
    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>   Change directory\n");
        printf("  exit       Exit the shell\n");
        printf("  help       Show this help message\n");
        printf("  jobs       List background jobs (not implemented yet)\n");
        printf("  history    Show history (use ↑↓ or !n)\n");
        return 1;
    }

    // jobs
    if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    // history
    if (strcmp(arglist[0], "history") == 0) {
        HIST_ENTRY **hist_list = history_list();
        if (hist_list) {
            for (int i = 0; hist_list[i]; i++)
                printf("%d  %s\n", i + 1, hist_list[i]->line);
        }
        return 1;
    }

    return 0;
}
