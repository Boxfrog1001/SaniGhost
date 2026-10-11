#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "shredder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define CHUNK_SIZE 65536

static int fill_random(uint8_t *buf, size_t size, int urandom_fd) {
    size_t total = 0;
    while (total < size) {
        ssize_t n = read(urandom_fd, buf + total, size - total);
        if (n <= 0) {
            if (n < 0 && (errno == EINTR || errno == EAGAIN)) continue;
            /* Fallback to rand() if urandom stream hiccups */
            for (size_t i = total; i < size; i++) {
                buf[i] = (uint8_t)(rand() & 0xFF);
            }
            return 0;
        }
        total += (size_t)n;
    }
    return 0;
}

int shred_file(const char *filepath, const ShredderConfig *config, SanitizeStats *stats) {
    if (!filepath || !config || !stats) return -1;

    struct stat st;
    if (stat(filepath, &st) != 0) {
        fprintf(stderr, "\033[1;31m[-] Skipping %s: %s\033[0m\n", filepath, strerror(errno));
        stats->errors++;
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        /* Skip non-regular files (symlinks, pipes, sockets) */
        return 0;
    }

    uint64_t size = (uint64_t)st.st_size;

    if (config->dry_run) {
        printf("\033[1;33m[DRY-RUN] Would shred (%lu bytes, %d passes): %s\033[0m\n", 
               (unsigned long)size, config->passes, filepath);
        stats->files_shredded++;
        stats->bytes_overwritten += size * config->passes;
        return 0;
    }

    if (size > 0) {
        int fd = open(filepath, O_WRONLY | O_SYNC);
        if (fd < 0) {
            /* Retry without O_SYNC if filesystem restricts synchronous flags */
            fd = open(filepath, O_WRONLY);
            if (fd < 0) {
                fprintf(stderr, "\033[1;31m[-] Failed to open %s for writing: %s\033[0m\n", filepath, strerror(errno));
                stats->errors++;
                return -1;
            }
        }

        uint8_t buffer[CHUNK_SIZE];
        int urandom_fd = open("/dev/urandom", O_RDONLY);

        for (int p = 1; p <= config->passes; p++) {
            if (lseek(fd, 0, SEEK_SET) < 0) {
                fprintf(stderr, "\033[1;31m[-] Seek error on %s: %s\033[0m\n", filepath, strerror(errno));
                stats->errors++;
                close(fd);
                if (urandom_fd >= 0) close(urandom_fd);
                return -1;
            }

            uint64_t bytes_left = size;
            while (bytes_left > 0) {
                size_t to_write = (bytes_left > CHUNK_SIZE) ? CHUNK_SIZE : (size_t)bytes_left;

                if (p == 1) {
                    memset(buffer, 0x00, to_write);
                } else if (p == 2 && config->passes >= 3) {
                    memset(buffer, 0xFF, to_write);
                } else {
                    if (urandom_fd >= 0) {
                        fill_random(buffer, to_write, urandom_fd);
                    } else {
                        for (size_t i = 0; i < to_write; i++) buffer[i] = (uint8_t)(rand() & 0xFF);
                    }
                }

                ssize_t written = write(fd, buffer, to_write);
                if (written < 0) {
                    if (errno == EINTR || errno == EAGAIN) continue;
                    fprintf(stderr, "\033[1;31m[-] Write failed on %s (pass %d): %s\033[0m\n", filepath, p, strerror(errno));
                    stats->errors++;
                    close(fd);
                    if (urandom_fd >= 0) close(urandom_fd);
                    return -1;
                }
                bytes_left -= (uint64_t)written;
            }

            /* Explicit unbuffered POSIX sync to persistent block storage */
            fdatasync(fd);
            stats->bytes_overwritten += size;
        }

        if (urandom_fd >= 0) close(urandom_fd);

        /* Truncate to 0 before unlinking */
        ftruncate(fd, 0);
        close(fd);
    }

    if (unlink(filepath) != 0) {
        fprintf(stderr, "\033[1;31m[-] Failed to unlink %s: %s\033[0m\n", filepath, strerror(errno));
        stats->errors++;
        return -1;
    }

    stats->files_shredded++;
    if (config->verbose) {
        printf("\033[1;32m[+] Shredded (%lu B, %d pass): %s\033[0m\n", (unsigned long)size, config->passes, filepath);
    }

    return 0;
}
