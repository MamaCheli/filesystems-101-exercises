#include <solution.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FILE_NAME 32

static const char *proc_path = "/proc";
// static const char *fd_path = "/fd";

void lsof(void)
{
    char proc_name[MAX_FILE_NAME], fd_name[MAX_FILE_NAME];
    char fd_path[PATH_MAX];
    char symlink[PATH_MAX + MAX_FILE_NAME];
    char openfile[PATH_MAX];

    int symlink_len;

    DIR *proc_dir;
    struct dirent *proc_entry;

    proc_dir = opendir(proc_path);
    if (proc_dir == NULL) {
        report_error(proc_path, errno);
        return;
    }

    while ((proc_entry = readdir(proc_dir)) != NULL) {
        strcpy(proc_name, proc_entry->d_name);
        if (atoi(proc_name) == 0) {
            continue;
        }

        snprintf(fd_path, sizeof(fd_path), "/proc/%s/fd", proc_name);

        DIR *fd_dir;
        struct dirent *fd_entry;

        fd_dir = opendir(fd_path);
        if (fd_dir == NULL) {
            report_error(fd_path, errno);
            continue;
        }

        while ((fd_entry = readdir(fd_dir)) != NULL) {
            strcpy(fd_name, fd_entry->d_name);
            if (!strcmp(fd_name, ".") || !strcmp(fd_name, "..")) {
                continue;
            }

            snprintf(symlink, sizeof(symlink), "%s/%s", fd_path, fd_name);

            symlink_len = readlink(symlink, openfile, sizeof(openfile));
            if (symlink_len < 0) {
                report_error(symlink, errno);
                continue;
            }
            openfile[symlink_len] = '\0';
            report_file(openfile);
        }
        closedir(fd_dir);
    }
    closedir(proc_dir);
}
