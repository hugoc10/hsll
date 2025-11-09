#define _POSIX_C_SOURCE 200809L // dprintf

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

enum RUNOPT {
        SYNC = 1,
        ASYNC = 0,
};

#ifndef strdup
#define strdup(s) memcpy(malloc(strlen((s)) + 1), (s), strlen((s)) + 1)
#endif

char *expand(char *);
void prompt();

static char *PROMPT = ">> ";
static bool alive = true;

void
prompt()
{
        printf("%s", expand(PROMPT));
}

char *
expand(char *str)
{
        return str;
}

/* ret and *ret must be free */
char **
split(char *str, char sep)
{
        char **arr = malloc(sizeof(char *));
        int arrlen = 1;
        char *c;

        arr[0] = c = strdup(str);
        for (; *c; c++) {
                if (*c != sep) continue;
                *c = 0;
                arr = realloc(arr, sizeof(char *) * (arrlen + 1));
                arr[arrlen] = c + 1;
                ++arrlen;
        }
        return arr;
}

void
freesplit(char **split)
{
        free(*split);
        free(split);
}

int
run(char **argv, int fdin, int fdout, bool sync, int *pid)
{
        int child;
        int status = 0;

        switch (child = fork()) {
        case -1:
                perror("fork");
                return -1;

        case 0:
                dup2(fdin, STDIN_FILENO);
                dup2(fdout, STDOUT_FILENO);

                status = execvp(argv[0], argv);
                dprintf(fdout, "%s: %s\n", argv[0], strerror(errno));
                exit(status);

        default:
                if (pid) *pid = child;
                if (sync) waitpid(child, &status, 0);
                return status;
        }
}

int
cd(char **argv)
{
        return chdir(argv[1]);
}

bool
is_builtin(char *cmd)
{
        return !strcmp("cd", cmd) ||
               !strcmp("exit", cmd) ||
               0; // style reasons
}

int
run_builtin(char **argv, int fdin, int fdout, bool sync, int *pid)
{
        int child;
        int status = 0;
        int fdinold, fdoutold;

        if (sync == ASYNC) {
                fprintf(stderr, "builtin cmd (%s) should be sync\n", *argv);
                return 1;
        }

        fdinold = dup(STDIN_FILENO);
        fdoutold = dup(STDOUT_FILENO);
        dup2(fdin, STDIN_FILENO);
        dup2(fdout, STDOUT_FILENO);

        // clang-format off
        if (0); // style reasons 
        else if (!strcmp("cd", *argv)) status = cd(argv);
        else if (!strcmp("exit", *argv)) alive = false;
        // clang-format on

        dup2(STDIN_FILENO, fdinold);
        dup2(STDOUT_FILENO, fdoutold);

        return status;
}

int
execute(char *cmd)
{
        char **scmd = split(cmd, ' ');

        (is_builtin(scmd[0]) ? run_builtin : run)
        /* Call the prev function with this arguments */
        (scmd, STDIN_FILENO, STDOUT_FILENO, SYNC, NULL);

        freesplit(scmd);
        return 0;
}

char *
getinput()
{
/*   */ #define SIZE 1024
        char *buf = malloc(SIZE);
        while (!fgets(buf, SIZE, stdin)) {
                prompt();
        }
        buf[SIZE - 1] = buf[strcspn(buf, "\n\r")] = 0;
        return buf;
}

int
main()
{
        while (alive) {
                prompt();
                execute(getinput());
        }
}
