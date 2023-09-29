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

    char exe[MAX_FILE_PATH];
    char *argv_buf[MAX_SIZE];
    char *envp_buf[MAX_SIZE];
    for (int i = 0; i < MAX_SIZE; i++) {
        argv_buf[i] = (char*) malloc(MAX_SIZE * sizeof(char));
        envp_buf[i] = (char*) malloc(MAX_SIZE * sizeof(char));
    }



    while ((entry = readdir(dir)) != NULL) {
        char proc_name[MAX_FILE_NAME];
        strcpy(proc_name, entry->d_name);

        if (atoi(proc_name) != 0) {
            pid_t pid = atoi(proc_name);
            char filepath[MAX_FILE_PATH];

            createPath(filepath, proc_name, exe_path);
            ssize_t exe_size = readlink(filepath, exe, sizeof(exe) - 1);
            if (exe_size == -1) {
                report_error(filepath, errno);
                continue;
            }
            exe[exe_size] = '\0';

            ///***********************************///

            createPath(filepath, proc_name, cmdline_path);
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

            createPath(filepath, proc_name, environ_path);
            FILE *environ_file = fopen(filepath, "r");
            if (environ_file == NULL) {
                report_error(filepath, errno);
                continue;
            }

            char *envp[MAX_SIZE];
            int env_count = 0;
            size_t env_size = MAX_SIZE;
            while (getdelim(&envp_buf[env_count], &env_size, '\0', cmdline_file) != -1 && envp_buf[env_count][0] != '\0') {
                envp[env_count] = envp_buf[env_count];
                env_count++;
            }
            envp[env_count] = NULL;

            fclose(cmdline_file);

            ///***********************************///

            report_process(pid, exe, argv, envp);
        }
    }

    for (int i = 0; i < MAX_SIZE; i++) {
        free(argv_buf[i]);
        free(envp_buf[i]);
    }
    closedir(dir);
}
