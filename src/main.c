#include "shell.h"
// Global history buffer
static char* history[HISTORY_SIZE];
static int history_count = 0;

/* -------------------------------------------------
   Add a command to history
   ------------------------------------------------- */
void add_to_history(const char* cmd) {
    if (cmd == NULL || strlen(cmd) == 0)
        return;

    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(cmd);
        history_count++;
    } else {
        // remove oldest and shift
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(cmd);
    }
}

/* -------------------------------------------------
   Show command history
   ------------------------------------------------- */
void show_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d  %s\n", i + 1, history[i]);
    }
}

/* -------------------------------------------------
   Retrieve command n (1-based)
   ------------------------------------------------- */
char* get_history_command(int n) {
    if (n <= 0 || n > history_count)
        return NULL;
    return strdup(history[n - 1]);
}

/* -------------------------------------------------
   Main loop
   ------------------------------------------------- */

int main() {
    char* cmdline;
    char** arglist;

    while (1) {
        cmdline = read_cmd(PROMPT, stdin);
        if (cmdline == NULL)
            break;

        // Handle !n before adding to history
        if (cmdline[0] == '!') {
            int num = atoi(cmdline + 1);
            char* recalled = get_history_command(num);
            if (recalled == NULL) {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            } else {
                printf("%s\n", recalled);
                free(cmdline);
                cmdline = recalled;
            }
        }

        if (cmdline[0] != '\0')
            add_to_history(cmdline);

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

    printf("\nShell exited.\n");
    return 0;
}
