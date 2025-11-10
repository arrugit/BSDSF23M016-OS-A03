#include "shell.h"

Job jobs[MAX_JOBS];
int job_count = 0;
pid_t foreground_pid = -1;
pid_t shell_pgid = -1;
struct termios shell_tmodes;

int main() {
    char* cmdline;
    char** arglist;

    /* initialize shell: put shell in its own pgid, grab terminal, ignore tty stop signals */
    init_shell();

    while (1) {
        check_background_jobs();

        cmdline = read_cmd(PROMPT);
        if (cmdline == NULL) {
            printf("\n");
            break;
        }

        if (strlen(cmdline) == 0) {
            free(cmdline);
            continue;
        }

        /* basic background detection */
        int background = 0;
        size_t len = strlen(cmdline);
        if (len > 0 && cmdline[len - 1] == '&') {
            background = 1;
            /* strip trailing & and any trailing space */
            cmdline[len - 1] = '\0';
            while (len > 1 && cmdline[len - 2] == ' ') {
                cmdline[len - 2] = '\0';
                len--;
            }
        }

        int cmdcount = 0;
        char*** cmdlist = parse_pipeline(cmdline, &cmdcount);

        if (cmdcount > 1) {
            execute_pipeline(cmdlist, cmdcount, background);
            for (int i = 0; i < cmdcount; i++) {
                if (cmdlist[i]) {
                    for (int j = 0; cmdlist[i][j] != NULL; j++)
                        free(cmdlist[i][j]);
                    free(cmdlist[i]);
                }
            }
            free(cmdlist);
        } else {
            arglist = tokenize(cmdline);
            if (arglist != NULL) {
                if (!handle_builtin(arglist)) {
                    execute_with_redirection(arglist, background);
                }
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
