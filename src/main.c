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

        if (strlen(cmdline) == 0) {
            free(cmdline);
            continue;
        }

        add_history(cmdline);

        int cmdcount = 0;
        char*** cmdlist = parse_pipeline(cmdline, &cmdcount);

        if (cmdcount > 1) {
            // Multiple commands: pipeline
            execute_pipeline(cmdlist, cmdcount);
            for (int i = 0; i < cmdcount; i++) {
                for (int j = 0; cmdlist[i][j] != NULL; j++)
                    free(cmdlist[i][j]);
                free(cmdlist[i]);
            }
            free(cmdlist);
        } else {
            // Single command: possible redirection
            arglist = tokenize(cmdline);
            if (arglist != NULL) {
                if (!handle_builtin(arglist))
                    execute_with_redirection(arglist);

                for (int i = 0; arglist[i] != NULL; i++)
                    free(arglist[i]);
                free(arglist);
            }
        }

        free(cmdline);
    }

    printf("Shell exited.\n");
    return 0;
}
