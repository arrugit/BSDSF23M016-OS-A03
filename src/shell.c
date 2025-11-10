#include "shell.h"

extern Job jobs[MAX_JOBS];
extern int job_count;
extern pid_t foreground_pid;
extern pid_t shell_pgid;
extern struct termios shell_tmodes;

/* ---------------- read_cmd (readline) ---------------- */
char* read_cmd(char* prompt) {
    char* input = readline(prompt);
    if (input == NULL)
        return NULL;
    if (strlen(input) > 0)
        add_history(input);
    return input;
}

/* ---------------- tokenize ---------------- */
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
        if (!arglist[i]) { perror("malloc failed"); exit(1); }
        memset(arglist[i], 0, ARGLEN);
    }

    char* saveptr = NULL;
    char* token = strtok_r(cmdline, " \t", &saveptr);
    int argnum = 0;
    while (token != NULL && argnum < MAXARGS) {
        strncpy(arglist[argnum], token, ARGLEN - 1);
        argnum++;
        token = strtok_r(NULL, " \t", &saveptr);
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

/* ---------------- parse_pipeline ---------------- */
char*** parse_pipeline(char* cmdline, int* cmdcount) {
    char* cmds[MAX_PIPE_CMDS];
    int count = 0;

    char* saveptr = NULL;
    char* token = strtok_r(cmdline, "|", &saveptr);
    while (token != NULL && count < MAX_PIPE_CMDS) {
        /* trim leading/trailing spaces */
        while (*token == ' ') token++;
        char* end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t')) { *end = '\0'; end--; }
        cmds[count++] = strdup(token);
        token = strtok_r(NULL, "|", &saveptr);
    }
    *cmdcount = count;

    char*** cmdlist = malloc(count * sizeof(char**));
    for (int i = 0; i < count; i++) {
        cmdlist[i] = tokenize(cmds[i]);
        free(cmds[i]);
    }

    return cmdlist;
}

/* ---------------- job functions ---------------- */
void add_job(pid_t pid, pid_t pgid, const char* cmd, JobStatus status) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        jobs[job_count].pgid = pgid;
        strncpy(jobs[job_count].command, cmd, sizeof(jobs[job_count].command) - 1);
        jobs[job_count].status = status;
        jobs[job_count].active = 1;
        printf("[%d] %d  %s\n", job_count + 1, (int)pid, cmd);
        job_count++;
    } else {
        fprintf(stderr, "Job list full\n");
    }
}

void check_background_jobs() {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].active && jobs[i].status != JOB_DONE) {
            int status;
            /* wait for any process in the job's pgid without blocking */
            pid_t r = waitpid(-jobs[i].pgid, &status, WNOHANG | WUNTRACED | WCONTINUED);
            if (r > 0) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    jobs[i].status = JOB_DONE;
                    jobs[i].active = 0;
                    printf("\n[%d] Done  %s\n", i + 1, jobs[i].command);
                } else if (WIFSTOPPED(status)) {
                    jobs[i].status = JOB_STOPPED;
                    printf("\n[%d] Stopped  %s\n", i + 1, jobs[i].command);
                } else if (WIFCONTINUED(status)) {
                    jobs[i].status = JOB_RUNNING;
                    /* keep active */
                }
            }
        }
    }
}

void list_jobs() {
    for (int i = 0; i < job_count; i++) {
        if (!jobs[i].active) continue;
        const char* s = (jobs[i].status == JOB_RUNNING) ? "Running" :
                        (jobs[i].status == JOB_STOPPED) ? "Stopped" : "Done";
        printf("[%d] %s  %s  (pgid=%d)\n", i + 1, s, jobs[i].command, (int)jobs[i].pgid);
    }
}

/* ---------------- helper parsers ---------------- */
/* job_index_from_token: token may be "%n" or a number "n" */
int job_index_from_token(const char* tok) {
    if (tok == NULL) return -1;
    if (tok[0] == '%') {
        return atoi(tok + 1); /* return 1-based index */
    } else {
        return atoi(tok); /* user may give number directly */
    }
}

/* find_job_by_index: index is 1-based; returns array index or -1 */
int find_job_by_index(int idx) {
    if (idx <= 0 || idx > job_count) return -1;
    if (!jobs[idx - 1].active) return -1;
    return idx - 1;
}

/* ---------------- init_shell & signals ---------------- */
void init_shell() {
    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        if (errno != EACCES && errno != EINVAL) perror("setpgid");
    }
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    tcgetattr(STDIN_FILENO, &shell_tmodes);

    /* ignore signals so shell doesn't get stopped */
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

/* optional: set up basic handlers if you want (we rely on check_background_jobs) */
void setup_signal_handlers() {
    /* nothing heavy here to avoid async-signal complexity; we poll in main loop */
}

/* ---------------- built-in commands ---------------- */
int fg_builtin(int job_index) {
    int idx = find_job_by_index(job_index);
    if (idx < 0) {
        fprintf(stderr, "fg: no such job %d\n", job_index);
        return -1;
    }
    Job *j = &jobs[idx];
    if (!j->active) { fprintf(stderr, "fg: job not active\n"); return -1; }

    pid_t pgid = j->pgid;
    /* give terminal to job */
    tcsetpgrp(STDIN_FILENO, pgid);

    /* send SIGCONT to the whole process group */
    if (kill(-pgid, SIGCONT) < 0) {
        perror("fg: kill(SIGCONT)");
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        return -1;
    }

    j->status = JOB_RUNNING;
    foreground_pid = j->pid;

    /* wait for the pgid (wait for any pid in pgid) */
    int status;
    pid_t w;
    /* wait until job finishes or stops */
    do {
        w = waitpid(-pgid, &status, WUNTRACED);
        if (w == -1) {
            if (errno == ECHILD) break;
            if (errno == EINTR) continue;
            perror("waitpid");
            break;
        }
        if (WIFSTOPPED(status)) {
            j->status = JOB_STOPPED;
            printf("\n[%d] Stopped  %s\n", idx + 1, j->command);
            break;
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        j->status = JOB_DONE;
        j->active = 0;
        /* report done already handled in check_background_jobs normally */
    }

    foreground_pid = -1;
    /* restore terminal to shell */
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    return 0;
}

int bg_builtin(int job_index) {
    int idx = find_job_by_index(job_index);
    if (idx < 0) {
        fprintf(stderr, "bg: no such job %d\n", job_index);
        return -1;
    }
    Job *j = &jobs[idx];
    if (!j->active) { fprintf(stderr, "bg: job not active\n"); return -1; }

    pid_t pgid = j->pgid;
    if (kill(-pgid, SIGCONT) < 0) {
        perror("bg: kill(SIGCONT)");
        return -1;
    }
    j->status = JOB_RUNNING;
    printf("[%d] %d resumed in background\n", idx + 1, (int)j->pid);
    return 0;
}

/* ---------------- built-in handler ---------------- */
int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL) return 0;

    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        char *dir = arglist[1];
        if (dir == NULL) dir = getenv("HOME");
        if (chdir(dir) != 0) perror("cd");
        return 1;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>     Change directory\n");
        printf("  exit         Exit shell\n");
        printf("  help         Show help message\n");
        printf("  history      Show command history\n");
        printf("  jobs         Show background jobs\n");
        printf("  fg %%n|n      Bring job n to foreground\n");
        printf("  bg %%n|n      Resume job n in background\n");
        printf("Supports redirection (<, >, >>), pipes (|), background (&), signals (Ctrl+C, Ctrl+Z)\n");
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

    if (strcmp(arglist[0], "fg") == 0) {
        /* syntax: fg [ %n | n ] */
        int job_index = 0;
        if (arglist[1] == NULL) {
            /* default to most recent job */
            job_index = job_count; /* may be invalid if last not active */
        } else {
            job_index = job_index_from_token(arglist[1]);
        }
        if (job_index <= 0) {
            fprintf(stderr, "fg: usage: fg %%n or fg n\n");
            return 1;
        }
        fg_builtin(job_index);
        return 1;
    }

    if (strcmp(arglist[0], "bg") == 0) {
        int job_index = 0;
        if (arglist[1] == NULL) {
            job_index = job_count;
        } else {
            job_index = job_index_from_token(arglist[1]);
        }
        if (job_index <= 0) {
            fprintf(stderr, "bg: usage: bg %%n or bg n\n");
            return 1;
        }
        bg_builtin(job_index);
        return 1;
    }

    return 0;
}
