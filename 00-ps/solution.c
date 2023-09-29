#include <solution.h>

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_SIZE 10000
#define MAX_FILE_NAME 32
#define MAX_FILE_PATH 256

const char *proc_path = "/proc";
const char *exe_path = "/exe";
const char *cmdline_path = "/cmdline";
const char *environ_path = "/environ";

void createPath(char *filepath, const char *pid, const char *filename) {
    strcpy(filepath, proc_path);
    strcat(filepath, "/");
    strcat(filepath, pid);
    strcat(filepath, filename);
}

void ps(void) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(proc_path);
    if (dir == NULL) {
        report_error(proc_path, errno);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char proc_name[MAX_FILE_NAME];
        strcpy(proc_name, entry->d_name);

        if (atoi(proc_name) != 0) {
            pid_t pid = atoi(proc_name);

            char exe[MAX_FILE_PATH];
            char *argv[MAX_SIZE];
            char *envp[MAX_SIZE];

            char filepath[MAX_FILE_PATH];

            createPath(filepath, proc_name, exe_path);
            ssize_t exe_size = readlink(filepath, exe, sizeof(exe) - 1);
            if (exe_size == -1) {
                report_error(filepath, errno);
                continue;
            }
            exe[exe_size] = '\0';

            char *buf[MAX_SIZE];

            createPath(filepath, proc_name, cmdline_path);
            FILE *cmdline_file = fopen(filepath, "r");
            if (cmdline_file == NULL) {
                report_error(filepath, errno);
                continue;
            }

            for (int i = 0; i < MAX_SIZE; i++) {
                buf[i] = (char *) malloc(MAX_SIZE * sizeof(char));
            }
            int arg_count = 0;

            while (fgets(buf[arg_count], sizeof(buf[arg_count]), cmdline_file) != NULL) {
                if (buf[arg_count][strlen(buf[arg_count]) - 1] == '\n') {
                    buf[arg_count][strlen(buf[arg_count]) - 1] = '\0';
                }
                argv[arg_count] = buf[arg_count];
                arg_count++;
            }
            argv[arg_count] = NULL;
            fclose(cmdline_file);

            for (int i = 0; i < MAX_SIZE; i++) {
                free(buf[i]);
            }

            createPath(filepath, proc_name, environ_path);
            FILE *environ_file = fopen(filepath, "r");
            if (environ_file == NULL) {
                report_error(filepath, errno);
                continue;
            }

            for (int i = 0; i < MAX_SIZE; i++) {
                buf[i] = (char *) malloc(MAX_SIZE * sizeof(char));
            }
            int env_count = 0;

            while (fgets(buf[env_count], sizeof(buf[env_count]), environ_file) != NULL) {
                if (buf[env_count][strlen(buf[env_count]) - 1] == '\n') {
                    buf[env_count][strlen(buf[env_count]) - 1] = '\0';
                }
                envp[env_count] = buf[env_count];
                env_count++;
            }
            envp[env_count] = NULL;
            fclose(environ_file);

            for (int i = 0; i < MAX_SIZE; i++) {
                free(buf[i]);
            }

            report_process(pid, exe, argv, envp);
        }
    }
    closedir(dir);
}
