#include <solution.h>

#include <sys/param.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

void abspath(const char *path) {
    char realpath[PATH_MAX];    

    size_t partpath_len, realpath_len;
    char partpath[PATH_MAX], token[PATH_MAX], symlink[PATH_MAX];
    // char tmp_str[PATH_MAX];

    struct stat stat_;
    int symlinks = 0;
    int symlink_len;

    realpath[0] = '\0';

    if (path[0] == '/') {
        realpath[0] = '/';
        realpath[1] = '\0';
        if (path[1] == '\0') {
            report_path(realpath);
            return;
        }
        realpath_len = 1;

        snprintf(partpath, sizeof(partpath), "%s", path + 1);
        partpath_len = strlen(partpath);
    } else {
        realpath_len = strlen(realpath);

        snprintf(partpath, sizeof(partpath), "%s", path);
        partpath_len = strlen(partpath);
    }

    if (partpath_len >= PATH_MAX || realpath_len >= PATH_MAX) {
        // report_error("/", partpath, ENAMETOOLONG);
        return;
    }

    while (partpath_len > 0) {
        char *first_slash = strchr(partpath, '/');
        char *token_ptr = first_slash ? first_slash : partpath + partpath_len;

        memmove(token, partpath, token_ptr - partpath);
        token[token_ptr - partpath] = '\0';
        partpath_len -= token_ptr - partpath;
        if (first_slash != NULL) {
            memmove(partpath, token_ptr + 1, partpath_len + 1);
        }

        if (realpath[realpath_len - 1] != '/') {
            if (realpath_len + 1 >= PATH_MAX) {
                report_error(realpath, token, ENAMETOOLONG);
                return;
            }
            realpath[realpath_len++] = '/';
            realpath[realpath_len] = '\0';
        }

        if (token[0] == '\0' || strcmp(token, ".") == 0) {
            continue;
        } else if (strcmp(token, "..") == 0) {
            if (realpath_len > 1) {
                realpath[realpath_len - 1] = '\0';
                char *last_slash = strrchr(realpath, '/') + 1;
                *last_slash = '\0';
                realpath_len = last_slash - realpath;
            }
            continue;
        }


        realpath_len = strlen(realpath);
        size_t token_len = strlen(token);
        for (size_t i = realpath_len; i < realpath_len + token_len; i++) {
            if (i >= PATH_MAX) {
                return;
            }
            realpath[i] = token[i - realpath_len];
        }
        realpath[realpath_len + token_len] = '\0';
        realpath_len = strlen(realpath);

        if (realpath_len >= PATH_MAX) {
            // report_error(realpath, token, ENAMETOOLONG);
            return;
        }

        if (lstat(realpath, &stat_) != 0) {
            if (errno == ENOENT && first_slash == NULL) {
                errno = 0;
                report_path(realpath);
            }
            return;
        }
        if (S_ISLNK(stat_.st_mode)) {
            if (symlinks++ > MAXSYMLINKS) {
                report_error(realpath, symlink, ELOOP);
                return;
            }
            symlink_len = readlink(realpath, symlink, sizeof(symlink) - 1);
            if (symlink_len < 0) {
                report_error(realpath, symlink, errno);
                return;
            }

            symlink[symlink_len] = '\0';
            if (symlink[0] == '/') {
                realpath[1] = 0;
                realpath_len = 1;
            } else if (realpath_len > 1) {
                realpath[realpath_len - 1] = '\0';
                char *last_slash = strrchr(realpath, '/') + 1;
                *last_slash = '\0';
                realpath_len = last_slash - realpath;
            }

            if (first_slash != NULL) {
                if (symlink[symlink_len - 1] != '/') {
                    // if (symlink_len + 1 >= sizeof(symlink)) {
                    //     report_error(realpath, symlink, ENAMETOOLONG);
                    //     return;
                    // }
                    symlink[symlink_len] = '/';
                    symlink[symlink_len + 1] = 0;
                }

                // snprintf(symlink, sizeof(partpath) + sizeof(symlink), "%s%s", symlink, partpath);
                // partpath_len = strlen(symlink);
                
                for (size_t i = symlink_len; i < symlink_len + partpath_len; i++) {
                    if (i >= PATH_MAX) {
                        return;
                    }
                    symlink[i] = partpath[i - symlink_len];
                }
                symlink[symlink_len + partpath_len] = '\0';
                partpath_len = strlen(symlink);

                // strcat(symlink, partpath);
                // partpath_len = strlen(symlink);

                if (partpath_len >= PATH_MAX) {
                    report_error(realpath, symlink, ENAMETOOLONG);
                    return;
                }
            }

            snprintf(partpath, sizeof(partpath), "%s", symlink);
            partpath_len = strlen(partpath);
        }
    }

    DIR* directory = opendir(realpath);
    if (directory) {
        if (realpath_len > 1 && realpath[realpath_len - 1] != '/') {
            realpath[realpath_len++] = '/';
            realpath[realpath_len] = '\0';
        }
    } else {
        if (realpath_len > 1 && realpath[realpath_len - 1] == '/') {
            realpath[realpath_len - 1] = '\0';
        }
    }
    closedir(directory);

    report_path(realpath);
    return;
}
