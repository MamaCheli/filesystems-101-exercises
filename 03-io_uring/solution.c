#include <solution.h>

#include <liburing.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define QUEUE_SIZE 4
#define BLOCK_MAX_SIZE (256 * 1024)

// struct io_data {
//     int is_read;
//     off_t first_offset, second_offset;
//     size_t size;
//     // struct iovec iov;
//     char* buf;
// };

// void queue_prepare(struct io_uring* ring, struct io_data* data, int infd, int outfd) {
//     struct io_uring_sqe* sqe;
//     sqe = io_uring_get_sqe(ring);
//     assert(sqe);

//     if (data->is_read) {
//         // io_uring_prep_readv(sqe, infd, &data->iov, 1, data->second_offset);
//         io_uring_prep_read(sqe, infd, data->buf + data->second_offset,
//                            data->size - data->second_offset,
//                            data->first_offset + data->second_offset);
//     } else {
//         // io_uring_prep_writev(sqe, outfd, &data->iov, 1, data->second_offset);
//         io_uring_prep_write(sqe, outfd, data->buf + data->second_offset,
//                             data->size - data->second_offset,
//                             data->first_offset + data->second_offset);
//     }

//     io_uring_sqe_set_data(sqe, data);
//     return;
// }

// int queue_read(struct io_uring* ring, off_t size, off_t offset, int infd) {
//     struct io_uring_sqe *sqe;
//     struct io_data *data;

//     if (!(data = malloc(size + sizeof(struct io_data)))) {
//         return 1;
//     }
//     if (!(sqe = io_uring_get_sqe(ring))) {
//         free(data);
//         return 1;
//     }

//     data->is_read = 1;
//     data->first_offset = offset;
//     data->second_offset = offset;
//     data->size = size;
//     // data->iov.iov_base = data + 1;
//     // data->iov.iov_len = size;
//     data->buf = (char*) data + sizeof(struct io_data);

//     // io_uring_prep_readv(sqe, infd, &data->iov, 1, offset);
//     io_uring_prep_read(sqe, infd, data->buf, size, offset);

//     io_uring_sqe_set_data(sqe, data);
//     return 0;
// }

// void queue_write(struct io_uring* ring, struct io_data* data, int infd, int outfd) {
//     data->is_read = 0;
//     data->second_offset = data->first_offset;
//     // data->iov.iov_base = data + 1;
//     // data->iov.iov_len = data->size;

//     queue_prepare(ring, data, infd, outfd);
//     // io_uring_prep_write(p_sqe, outfd, data->buf, data->size, data->first_offset);
//     io_uring_submit(ring);
//     return;
// }

struct io_data {
    int is_read;
    off_t first_offset, second_offset;
    size_t size;
    // struct iovec iov;
    char* buf;
};

void queue_prepare(struct io_uring* ring, struct io_data* data, int infd, int outfd, off_t process_len) {
    struct io_uring_sqe* sqe;
    sqe = io_uring_get_sqe(ring);
    assert(sqe);

    data->second_offset += process_len;

    if (data->is_read) {
        // io_uring_prep_readv(sqe, infd, &data->iov, 1, data->second_offset);
        io_uring_prep_read(sqe, infd, data->buf + data->second_offset,
                           data->size - data->second_offset,
                           data->first_offset + data->second_offset);
    } else {
        // io_uring_prep_writev(sqe, outfd, &data->iov, 1, data->second_offset);
        io_uring_prep_write(sqe, outfd, data->buf + data->second_offset,
                            data->size - data->second_offset,
                            data->first_offset + data->second_offset);
    }

    io_uring_sqe_set_data(sqe, data);
    return;
}

int queue_read(struct io_uring* ring, off_t size, off_t offset, int infd) {
    struct io_uring_sqe *sqe;
    struct io_data *data;

    if (!(data = malloc(size + sizeof(struct io_data)))) {
        return 1;
    }
    if (!(sqe = io_uring_get_sqe(ring))) {
        free(data);
        return 1;
    }

    data->is_read = 1;
    data->first_offset = offset;
    data->second_offset = 0;
    data->size = size;
    // data->iov.iov_base = data + 1;
    // data->iov.iov_len = size;
    data->buf = (char*) data + sizeof(struct io_data);

    // io_uring_prep_readv(sqe, infd, &data->iov, 1, offset);
    io_uring_prep_read(sqe, infd, data->buf, size, offset);
    io_uring_sqe_set_data(sqe, data);
    return 0;
}

void queue_write(struct io_uring* ring, struct io_data* data, int outfd) {
    data->is_read = 0;
    data->second_offset = 0;
    // data->iov.iov_base = data + 1;
    // data->iov.iov_len = data->size;

    struct io_uring_sqe* sqe = io_uring_get_sqe(ring);
    assert(sqe);

    io_uring_prep_write(sqe, outfd, data->buf, data->size, data->first_offset);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(ring);
}

///////////////////////////////////////////////////////////////////////////////

int copy(int in, int out) {
    int reads = 0;
    int writes = 0;
    off_t read_size, write_size;
    off_t offset = 0;

    struct io_uring_cqe* cqe;
    struct io_uring ring;

    int errno_code;
    errno_code = io_uring_queue_init(QUEUE_SIZE, &ring, 0);
    if (errno_code < 0) {
        return errno_code;
    }

    struct stat stat_;
    if (fstat(in, &stat_)) {
        errno_code = -errno;
        return errno_code;
    }
    read_size = stat_.st_size;
    write_size = read_size;

    while (read_size > 0 || write_size > 0) {
        int old_reads = reads;
        while (read_size > 0) {
            off_t current_size = read_size;

            if (reads + writes >= QUEUE_SIZE) {
                break;
            }
            if (current_size > BLOCK_MAX_SIZE) {
                current_size = BLOCK_MAX_SIZE;
            } else if (!current_size) {
                break;
            }

            if (queue_read(&ring, current_size, offset, in)) {
                break;
            }

            read_size -= current_size;
            offset += current_size;
            reads++;
        }

        if (old_reads != reads) {
            errno_code = io_uring_submit(&ring);
            if (errno_code < 0) {
                return errno_code;
            }
        }

        int wait_flag = 1;
        while (write_size > 0) {
            struct io_data* data;

            if (wait_flag) {
                errno_code = io_uring_wait_cqe(&ring, &cqe);
                wait_flag = 0;
            } else {
                errno_code = io_uring_peek_cqe(&ring, &cqe);
                if (errno_code == -EAGAIN) {
                    cqe = NULL;
                    errno_code = 0;
                }
            }
            if (errno_code < 0) {
                return errno_code;
            }
            if (!cqe) {
                break;
            }

            data = io_uring_cqe_get_data(cqe);
            if (cqe->res < 0) {
                if (cqe->res == -EAGAIN) {
                    queue_prepare(&ring, data, in, out, 0);
                    io_uring_cqe_seen(&ring, cqe);
                    continue;
                }
                return cqe->res;
            } else if ((size_t) cqe->res != data->size) {
                queue_prepare(&ring, data, in, out, cqe->res); 
                io_uring_cqe_seen(&ring, cqe);
                continue;
            }

            if (data->is_read) {
                queue_write(&ring, data, out);
                write_size -= data->size;
                reads--;
                writes++;
            } else {
                free(data);
                writes--;
            }
            io_uring_cqe_seen(&ring, cqe);
        }
    }

    while (writes > 0) {
        struct io_data* data;

        errno_code = io_uring_wait_cqe(&ring, &cqe);
        if (errno_code < 0) {
            return errno_code;
        }
        if (cqe->res < 0) {
            return cqe->res;
        }

        data = io_uring_cqe_get_data(cqe);
        free(data);
        writes--;
        io_uring_cqe_seen(&ring, cqe);
    }

    return 0;
}
