#include <solution.h>

#include <fuse3/fuse.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

static void *hellofs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    (void) conn;
    (void) cfg;

    return NULL;
}

static int hellofs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                          struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void)offset;
    (void)fi;
    (void)flags;

    if (strcmp(path, "/") == 0) {
        filler(buf, ".", NULL, 0, 0);
        filler(buf, "..", NULL, 0, 0);
        filler(buf, "hello", NULL, 0, 0);
        return 0;
    } 
    return -ENOENT;
}

static int hellofs_getattr(const char *path, struct stat *stbuf,
                          struct fuse_file_info *fi) {
    (void)fi;

    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0){
        stbuf->st_mode = S_IFDIR;
        return 0;
    } 
    if(strcmp(path, "/hello") == 0){
        stbuf->st_mode = S_IFREG | S_IRUSR;
        stbuf->st_size = 20;
        return 0;
    }
    return -ENOENT;
}

static int hellofs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void)path;
    (void)mode;
    (void)fi;

    return -EROFS;
}

static int hellofs_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/hello") != 0) {
        return -ENOENT;
    }
    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        return -EROFS;
    }    
    return 0; 
}

static int hellofs_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    (void)fi;

    if (strcmp(path, "/hello") != 0) {
        return -ENOENT;
    }

    char hello_pid[20];
    memset(hello_pid, 0, sizeof(hello_pid));
    
    if (offset > (off_t)size) {
        return 0;
    }
    snprintf(hello_pid, 20, "hello, %d\n", fuse_get_context()->pid);
    size = (size <= strlen(hello_pid) - offset ? size : strlen(hello_pid) - offset);
    memcpy(buf, hello_pid + offset, size);
    return size;
}

static int hellofs_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi) {
    (void)path;
    (void)buf;
    (void)size;
    (void)offset;
    (void)fi;

    return -EROFS;
}

static const struct fuse_operations hellofs_ops = {
    .init = hellofs_init,
    .readdir = hellofs_readdir,
    .getattr = hellofs_getattr,
    .create = hellofs_create,
    .open = hellofs_open,
    .read = hellofs_read,
    .write = hellofs_write
};

int helloworld(const char *mntp)
{
    char *argv[] = {"exercise", "-f", (char *)mntp, NULL};
    return fuse_main(3, argv, &hellofs_ops, NULL);
}