/*******************************************************************************
 * Name        : sl.c
 * Author      : Samantha Bryan
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <DIRECTORY>\n", argv[0]);
        fflush(stderr);
        return EXIT_FAILURE;
    }

    const char *dir = argv[1];

    // Check read accessibility
    if (access(dir, R_OK) != 0)
    {
        fprintf(stderr, "Permission denied. %s cannot be read.", dir);
        fflush(stderr);
        fsync(STDERR_FILENO);
        return EXIT_FAILURE;
    }

    // Check that the path is actually a directory
    struct stat st;
    if (stat(dir, &st) != 0)
    {
        fprintf(stderr, "Permission denied. %s cannot be read.", dir);
        fflush(stderr);
        fsync(STDERR_FILENO);
        return EXIT_FAILURE;
    }
    if (!S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "The first argument has to be a directory.");
        fflush(stderr);
        return EXIT_FAILURE;
    }

    int pipe1[2]; // ls -> sort
    int pipe2[2]; // sort -> parent

    if (pipe(pipe1) != 0)
    {
        perror("pipe1");
        return EXIT_FAILURE;
    }
    if (pipe(pipe2) != 0)
    {
        perror("pipe2");
        return EXIT_FAILURE;
    }

    // Child 1: ls -ai <DIRECTORY>
    pid_t pid_ls = fork();
    if (pid_ls < 0)
    {
        perror("fork ls");
        return EXIT_FAILURE;
    }

    if (pid_ls == 0)
    {
        if (dup2(pipe1[1], STDOUT_FILENO) < 0)
        {
            perror("dup2 ls stdout");
            exit(EXIT_FAILURE);
        }

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        execlp("ls", "ls", "-ai", dir, (char *)NULL);

        fprintf(stderr, "Error: ls failed.\n");
        exit(EXIT_FAILURE);
    }

    // Child 2: sort -n
    pid_t pid_sort = fork();
    if (pid_sort < 0)
    {
        perror("fork sort");
        return EXIT_FAILURE;
    }

    if (pid_sort == 0)
    {
        if (dup2(pipe1[0], STDIN_FILENO) < 0)
        {
            perror("dup2 sort stdin");
            exit(EXIT_FAILURE);
        }

        if (dup2(pipe2[1], STDOUT_FILENO) < 0)
        {
            perror("dup2 sort stdout");
            exit(EXIT_FAILURE);
        }

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        execlp("sort", "sort", (char *)NULL);

        fprintf(stderr, "Error: sort failed.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[1]);

    char buf[4096];
    ssize_t n;
    int total_files = 0;

    while ((n = read(pipe2[0], buf, sizeof(buf))) > 0)
    {
        // count number of file entries
        for (ssize_t i = 0; i < n; i++)
        {
            if (buf[i] == '\n')
                total_files++;
        }

        if (write(STDOUT_FILENO, buf, (size_t)n) < 0)
        {
            perror("write");
            fflush(stderr);
            break;
        }
    }
    if (n < 0)
    {
        perror("read pipe2");
        fflush(stderr);
    }

    close(pipe2[0]);

    // Print total file count
    printf("Total files: %d\n", total_files);

    // Wait for both children
    int status;
    if (waitpid(pid_ls, &status, 0) < 0)
    {
        perror("waitpid ls");
        return EXIT_FAILURE;
    }
    if (waitpid(pid_sort, &status, 0) < 0)
    {
        perror("waitpid sort");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}