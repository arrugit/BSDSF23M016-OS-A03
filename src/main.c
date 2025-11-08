#include "shell.h"

Job jobs[MAX_JOBS];
int job_count = 0;

int main() {
    char* cmdline;
    char** arglist;

    signal(SIGCHLD, SIG_IGN); // prevent zombie processes

    while (1) {
        check_background_jobs(); // cleanup finished jobs
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

        int background = 0;
        if (cmdline[strlen(cmdline) - 1] == '&') {
            background = 1;
            cmdline[strlen(cmdline) - 1] = '\0'; // remove '&'
        }

        int cmdcount = 0;
        char*** cmdlist = parse_pipeline(cmdline, &cmdcount);

        if (cmdcount > 1) {
            execute_pipeline(cmdlist, cmdcount, background);
            for (int i = 0; i < cmdcount; i++) {
                for (int j = 0; cmdlist[i][j] != NULL; j++)
                    free(cmdlist[i][j]);
                free(cmdlist[i]);
            }
            free(cmdlist);
        } else {
            arglist = tokenize(cmdline);
            if (arglist != NULL) {
                if (!handle_builtin(arglist))
                    execute_with_redirection(arglist, background);

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
