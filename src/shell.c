#include "shell.h"

/* -----------------------------------------------------
   Read command line input from user
   ----------------------------------------------------- */
char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }

    cmdline[pos] = '\0';
    return cmdline;
}

/* -----------------------------------------------------
   Tokenize input string into arguments
   ----------------------------------------------------- */
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

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

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;
        if (*cp == '\0') break;

        start = cp;
        len = 0;
        while (*cp != '\0' && *cp != ' ' && *cp != '\t') {
            cp++;
            len++;
        }

        if (len >= ARGLEN)
            len = ARGLEN - 1;  // prevent overflow

        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
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
        return 1;
    }

    // jobs (placeholder)
    if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    return 0; // not a built-in
}
