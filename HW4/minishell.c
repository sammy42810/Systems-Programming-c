/*******************************************************************************
 * Name        : minishell.c
 * Author      : Samantha Bryan
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>

#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

// So the main loop can react to Ctrl-C
volatile sig_atomic_t interrupted = 0;

// Sets the interrupted flag rather than terminating, so the shell can cancel the current input and re-display the prompt
void sigint_handler(int sig)
{
    (void)sig;
    interrupted = 1;
}

// Returns the home directory
char *get_home_dir(void)
{
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL)
    {
        perror("getpwuid");
        return NULL;
    }
    return pw->pw_dir;
}

// Built-in "cd" command with support for "cd", "cd ~", and "cd ~/path"
void cd(char **args, int argc)
{
    if (argc > 2)
    {
        fprintf(stderr, "Error: Too many arguments to cd.\n");
        return;
    }

    char *target;
    char path_buf[PATH_MAX];

    if (argc == 1 || strcmp(args[1], "~") == 0)
    {
        target = get_home_dir();
        if (target == NULL)
            return;
    }
    else if (args[1][0] == '~')
    {
        char *home = get_home_dir();
        if (home == NULL)
            return;
        snprintf(path_buf, sizeof(path_buf), "%s%s", home, args[1] + 1);
        target = path_buf;
    }
    else
    {
        target = args[1];
    }

    if (chdir(target) != 0)
    {
        fprintf(stderr, "Error: Cannot change directory to %s. %s.\n",
                args[1] ? args[1] : "~", strerror(errno));
    }
}

// Built-in "pwd" command
void pwd(void)
{
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd");
        return;
    }
    printf("%s\n", cwd);
}

// Built-in "lf" command
void lf(void)
{
    DIR *dir = opendir(".");
    if (dir == NULL)
    {
        perror("opendir");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        printf("%s\n", entry->d_name);
    }
    closedir(dir);
}

// Holds information about a single process for the "lp" command
typedef struct
{
    int pid;
    char user[256];
    char cmd[4096];
} ProcessInfo;

// Checks if number
int is_number(const char *s)
{
    while (*s)
    {
        if (!isdigit((unsigned char)*s))
            return 0;
        s++;
    }
    return 1;
}

// Comparison function for qsort
int cmp_proc(const void *a, const void *b)
{
    return ((ProcessInfo *)a)->pid - ((ProcessInfo *)b)->pid;
}

// Built-in "lp" command
void lp(void)
{
    DIR *dir = opendir("/proc");
    if (dir == NULL)
    {
        perror("opendir");
        return;
    }

    ProcessInfo *procs = NULL;
    int count = 0;
    int capacity = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (!is_number(entry->d_name))
            continue;

        int pid = atoi(entry->d_name);
        char proc_path[512];
        snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);

        struct stat st;
        if (stat(proc_path, &st) != 0)
            continue;

        struct passwd *pw = getpwuid(st.st_uid);

        char cmdline_path[512];
        snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);
        FILE *f = fopen(cmdline_path, "r");
        char cmd[4096] = "";
        if (f != NULL)
        {
            size_t n = fread(cmd, 1, sizeof(cmd) - 1, f);
            for (size_t i = 0; i < n; i++)
            {
                if (cmd[i] == '\0')
                    cmd[i] = ' ';
            }
            if (n > 0 && cmd[n - 1] == ' ')
                cmd[n - 1] = '\0';
            fclose(f);
        }

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 256 : capacity * 2;
            procs = realloc(procs, capacity * sizeof(ProcessInfo));
            if (procs == NULL)
            {
                perror("realloc");
                closedir(dir);
                return;
            }
        }

        procs[count].pid = pid;
        strncpy(procs[count].user, pw ? pw->pw_name : "unknown", 255);
        procs[count].user[255] = '\0';
        strncpy(procs[count].cmd, cmd, 4095);
        procs[count].cmd[4095] = '\0';
        count++;
    }
    closedir(dir);

    qsort(procs, count, sizeof(ProcessInfo), cmp_proc);

    for (int i = 0; i < count; i++)
    {
        printf("%5d %s %s\n", procs[i].pid, procs[i].user, procs[i].cmd);
    }
    free(procs);
}

// Tokenizes the input line on whitespace and fills the args array
int parse_args(char *line, char **args, int max_args)
{
    int argc = 0;
    char *token = strtok(line, " \t\n");
    while (token != NULL && argc < max_args - 1)
    {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    return argc;
}

// Forks a child process and uses execvp to run an external command
// The parent waits for the child to finish
void exec_command(char **args)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return;
    }
    if (pid == 0)
    {
        if (execvp(args[0], args) < 0)
        {
            perror("execv");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        int status;
        if (waitpid(pid, &status, 0) < 0)
        {
            if (errno != EINTR)
                perror("waitpid");
        }
    }
}

int main(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) < 0)
    {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    char buf[4096];
    char cwd[4096];
    char *args[256];

    while (1)
    {
        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            perror("getcwd");
            continue;
        }
        printf("%s[%s]> %s", BLUE, cwd, DEFAULT);
        fflush(stdout);

        ssize_t bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (interrupted)
        {
            interrupted = 0;
            printf("\n");
            continue;
        }
        if (bytes <= 0)
        {
            if (bytes < 0 && errno == EINTR)
                continue;
            break;
        }
        buf[bytes] = '\0';

        // strip trailing newline
        if (bytes > 0 && buf[bytes - 1] == '\n')
            buf[bytes - 1] = '\0';

        // Ignore blank lines
        if (buf[0] == '\0')
            continue;

        int arg_count = parse_args(buf, args, 256);
        if (arg_count == 0)
            continue;

        if (strcmp(args[0], "exit") == 0)
        {
            return EXIT_SUCCESS;
        }
        else if (strcmp(args[0], "cd") == 0)
        {
            cd(args, arg_count);
        }
        else if (strcmp(args[0], "pwd") == 0)
        {
            pwd();
        }
        else if (strcmp(args[0], "lf") == 0)
        {
            lf();
        }
        else if (strcmp(args[0], "lp") == 0)
        {
            lp();
        }
        else
        {
            exec_command(args);
        }
    }
    return EXIT_SUCCESS;
}
