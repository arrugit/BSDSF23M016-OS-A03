#include "shell.h"

Job jobs[MAX_JOBS];
int job_count = 0;
pid_t foreground_pid = -1;
pid_t shell_pgid = -1;
struct termios shell_tmodes;

int main() {
    char* cmdline;
    char** arglist;

    init_shell();
    setup_signal_handlers();

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

        /* background detection by trailing & (single command or pipeline) */
        int background = 0;
        size_t len = strlen(cmdline);
        if (len > 0) {
            /* strip trailing spaces */
            while (len > 0 && (cmdline[len - 1] == ' ' || cmdline[len - 1] == '\t'))
                cmdline[--len] = '\0';
        }
        if (len > 0 && cmdline[len - 1] == '&') {
            background = 1;
            cmdline[len - 1] = '\0';
            /* strip trailing spaces again */
            while (len > 1 && (cmdline[len - 2] == ' ' || cmdline[len - 2] == '\t')) {
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
            /* single command */
            arglist = tokenize(cmdline);
            if (arglist != NULL) {
                /* builtins (fg/bg/jobs/cd/exit/help/history) */
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
