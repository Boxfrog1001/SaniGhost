#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "direct_io_bypass.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int open_direct_io_target(const char *path) {
    /* O_DIRECT forces the Linux kernel to bypass the page cache completely,
     * transferring data directly from user space buffers to the device driver. */
    int fd = open(path, O_RDWR | O_DIRECT | O_SYNC);
    if (fd < 0) {
        /* Fallback to standard O_SYNC if filesystem does not support O_DIRECT (e.g. tmpfs) */
        fd = open(path, O_RDWR | O_SYNC);
    }
    return fd;
}

void *alloc_aligned_block_buffer(size_t size, size_t alignment) {
    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return ptr;
}

void free_aligned_block_buffer(void *ptr) {
    free(ptr);
}
