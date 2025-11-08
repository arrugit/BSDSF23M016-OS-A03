#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while (1) {
        cmdline = read_cmd(PROMPT);
        if (cmdline == NULL) {
            printf("\n");
            break;
        }

        // Skip empty commands
        if (strlen(cmdline) == 0) {
            free(cmdline);
            continue;
        }

        // Store in history
        add_history(cmdline);

        if ((arglist = tokenize(cmdline)) != NULL) {
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }

            for (int i = 0; arglist[i] != NULL; i++)
                free(arglist[i]);
            free(arglist);
        }

        free(cmdline);
    }

    printf("Shell exited.\n");
    return 0;
}
