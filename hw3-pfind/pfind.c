/*******************************************************************************
 * Name        : pfind.c
 * Author      : Samantha Bryan
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

static const mode_t PERM_MASKS[9] = {
    S_IRUSR, S_IWUSR, S_IXUSR,
    S_IRGRP, S_IWGRP, S_IXGRP,
    S_IROTH, S_IWOTH, S_IXOTH};
static const char PERM_CHARS[9] = {
    'r', 'w', 'x',
    'r', 'w', 'x',
    'r', 'w', 'x'};

int validate_pstring(const char *pstring)
{
    if (strlen(pstring) != 9)
        return 0;
    for (int i = 0; i < 9; i++)
    {
        if (pstring[i] != PERM_CHARS[i] && pstring[i] != '-')
            return 0;
    }
    return 1;
}

mode_t pstring_to_mode(const char *pstring)
{
    mode_t mode = 0;
    for (int i = 0; i < 9; i++)
    {
        if (pstring[i] == PERM_CHARS[i])
            mode |= PERM_MASKS[i];
    }
    return mode;
}

void find_files(const char *dirpath, mode_t target_mode)
{
    DIR *dp = opendir(dirpath);
    if (dp == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            errno = 0;
            continue;
        }

        size_t path_len = strlen(dirpath) + 1 + strlen(entry->d_name) + 1;
        char *fullpath = malloc(path_len);
        if (fullpath == NULL)
        {
            perror("malloc");
            closedir(dp);
            exit(EXIT_FAILURE);
        }
        snprintf(fullpath, path_len, "%s%s%s",
                 dirpath,
                 (dirpath[strlen(dirpath) - 1] == '/') ? "" : "/",
                 entry->d_name);

        struct stat sb;
        if (lstat(fullpath, &sb) == -1)
        {
            perror("lstat");
            free(fullpath);
            exit(EXIT_FAILURE);
        }

        if (S_ISDIR(sb.st_mode))
        {
            find_files(fullpath, target_mode);
        }
        else if (S_ISREG(sb.st_mode))
        {
            mode_t file_perms = sb.st_mode & 0777;
            if (file_perms == target_mode)
                printf("%s\n", fullpath);
        }

        free(fullpath);
        errno = 0;
    }

    if (errno != 0)
        perror("readdir");

    if (closedir(dp) == -1)
        perror("closedir");
}

int main(int argc, char *argv[])
{
    (void)argc;
    const char *directory = argv[1];
    const char *pstring = argv[2];

    if (!validate_pstring(pstring))
    {
        fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", pstring);
        return EXIT_FAILURE;
    }
    mode_t target = pstring_to_mode(pstring);
    find_files(directory, target);

    return EXIT_SUCCESS;
}