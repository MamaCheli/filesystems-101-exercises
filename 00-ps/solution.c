#include <solution.h>

#include <fs_malloc.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_SIZE 10000
#define MAX_FILE_NAME 32
#define MAX_FILE_PATH 256

static const char *proc_path = "/proc";
static const char *exe_path = "/exe";
static const char *cmdline_path = "/cmdline";
static const char *environ_path = "/environ";

void ps(void) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(proc_path);
    if (dir == NULL) {
        report_error(proc_path, errno);
        return;
    }

    char exe[MAX_FILE_PATH];
    char *argv_buf[MAX_SIZE];
    char *envp_buf[MAX_SIZE];
    for (int i = 0; i < MAX_SIZE; i++) {
        argv_buf[i] = fs_xmalloc(MAX_SIZE);
        envp_buf[i] = fs_xmalloc(MAX_SIZE);
    }

    while ((entry = readdir(dir)) != NULL) {
        char proc_name[MAX_FILE_NAME];
        strcpy(proc_name, entry->d_name);

        if (atoi(proc_name) == 0) {
            continue;
        }

        pid_t pid = atoi(proc_name);
        char filepath[MAX_FILE_PATH];

        snprintf(filepath, sizeof(filepath), "/proc/%s/%s", proc_name, exe_path);
        ssize_t exe_size = readlink(filepath, exe, sizeof(exe) - 1);
        if (exe_size == -1) {
            report_error(filepath, errno);
            continue;
        }
        exe[exe_size] = '\0';

        ///***********************************///

        snprintf(filepath, sizeof(filepath), "/proc/%s/%s", proc_name, cmdline_path);
        FILE *cmdline_file = fopen(filepath, "r");
        if (cmdline_file == NULL) {
            report_error(filepath, errno);
            continue;
        }

        char *argv[MAX_SIZE];
        int arg_count = 0;
        size_t arg_size = MAX_SIZE;
        while (getdelim(&argv_buf[arg_count], &arg_size, '\0', cmdline_file) != -1 && argv_buf[arg_count][0] != '\0') {
            argv[arg_count] = argv_buf[arg_count];
            arg_count++;
        }
        argv[arg_count] = NULL;

        fclose(cmdline_file);

        ///***********************************///

        snprintf(filepath, sizeof(filepath), "/proc/%s/%s", proc_name, environ_path);
        FILE *environ_file = fopen(filepath, "r");
        if (environ_file == NULL) {
            report_error(filepath, errno);
            continue;
        }

        char *envp[MAX_SIZE];
        int env_count = 0;
        size_t env_size = MAX_SIZE;
        while (getdelim(&envp_buf[env_count], &env_size, '\0', environ_file) != -1 && envp_buf[env_count][0] != '\0') {
            envp[env_count] = envp_buf[env_count];
            env_count++;
        }
        envp[env_count] = NULL;

        fclose(environ_file);

        ///***********************************///

        report_process(pid, exe, argv, envp);
    }

    for (int i = 0; i < MAX_SIZE; i++) {
        fs_xfree(argv_buf[i]);
        fs_xfree(envp_buf[i]);
    }
    closedir(dir);
}
