#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "shredder.h"
#include "thumbnail_wipe.h"
#include "direct_io_bypass.h"
#include "ftl_pressure_flush.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
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

    /* Cluster Slack Space Intercept:
     * Round target overwrite size up to the nearest filesystem block/cluster boundary
     * (st.st_blksize, typically 4096 bytes) to scrub residual slack fragments.
     */
    blksize_t blk = (st.st_blksize > 0) ? st.st_blksize : 4096;
    uint64_t overwrite_size = ((size + (uint64_t)blk - 1) / (uint64_t)blk) * (uint64_t)blk;
    if (overwrite_size == 0 && size > 0) overwrite_size = size;

    if (config->dry_run) {
        printf("\033[1;33m[DRY-RUN] Would shred (%lu bytes [slack padded: %lu B], %d passes): %s\033[0m\n", 
               (unsigned long)size, (unsigned long)overwrite_size, config->passes, filepath);
        stats->files_shredded++;
        stats->bytes_overwritten += overwrite_size * config->passes;
        return 0;
    }

    if (size > 0) {
        /* Level 3: Attempt direct kernel page cache bypass (O_DIRECT | O_SYNC) */
        int fd = open_direct_io_target(filepath);
        bool is_direct_io = true;
        if (fd < 0 && (errno == EACCES || errno == EPERM)) {
            chmod(filepath, st.st_mode | S_IWUSR);
            fd = open_direct_io_target(filepath);
        }

        if (fd < 0) {
            /* Fallback to standard synchronous I/O if filesystem prohibits O_DIRECT */
            is_direct_io = false;
            fd = open(filepath, O_WRONLY | O_SYNC);
            if (fd < 0 && (errno == EACCES || errno == EPERM)) {
                chmod(filepath, st.st_mode | S_IWUSR);
                fd = open(filepath, O_WRONLY | O_SYNC);
            }
            if (fd < 0) {
                fd = open(filepath, O_WRONLY);
                if (fd < 0 && (errno == EACCES || errno == EPERM)) {
                    chmod(filepath, st.st_mode | S_IWUSR);
                    fd = open(filepath, O_WRONLY);
                }
            }
            if (fd < 0) {
                fprintf(stderr, "\033[1;31m[-] Failed to open %s for writing: %s\033[0m\n", filepath, strerror(errno));
                stats->errors++;
                return -1;
            }
        }

        /* Sector-aligned DMA buffer for O_DIRECT */
        uint8_t *aligned_buf = (uint8_t *)alloc_aligned_block_buffer(CHUNK_SIZE, 4096);
        uint8_t fallback_buf[CHUNK_SIZE];
        uint8_t *buffer = aligned_buf ? aligned_buf : fallback_buf;
        int urandom_fd = open("/dev/urandom", O_RDONLY);

        for (int p = 1; p <= config->passes; p++) {
            if (lseek(fd, 0, SEEK_SET) < 0) {
                fprintf(stderr, "\033[1;31m[-] Seek error on %s: %s\033[0m\n", filepath, strerror(errno));
                stats->errors++;
                close(fd);
                if (urandom_fd >= 0) close(urandom_fd);
                if (aligned_buf) free_aligned_block_buffer(aligned_buf);
                return -1;
            }

            uint64_t bytes_left = overwrite_size;
            while (bytes_left > 0) {
                size_t to_write = (bytes_left > CHUNK_SIZE) ? CHUNK_SIZE : (size_t)bytes_left;
                /* If using O_DIRECT, size must align to 512 or 4096 */
                if (is_direct_io && (to_write % 4096 != 0)) {
                    to_write = ((to_write + 4095) / 4096) * 4096;
                }

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
                    if (aligned_buf) free_aligned_block_buffer(aligned_buf);
                    return -1;
                }
                if ((uint64_t)written >= bytes_left) {
                    bytes_left = 0;
                } else {
                    bytes_left -= (uint64_t)written;
                }
            }

            fdatasync(fd);
            stats->bytes_overwritten += overwrite_size;
        }

        if (urandom_fd >= 0) close(urandom_fd);
        if (aligned_buf) free_aligned_block_buffer(aligned_buf);

        /* Truncate to 0 before final removal */
        ftruncate(fd, 0);
        close(fd);
    }

    /* ---- Chain‑of‑Custody Deletion Steps ---- */

    /* 🔍 FreeDesktop Thumbnail Cache Scrub:
     * Check if desktop environments (GNOME, KDE, XFCE) cached preview thumbnails of this file
     * in ~/.cache/thumbnails/ and wipe them automatically to eliminate preview leakage.
     */
    if (strstr(filepath, "/.cache/thumbnails/") == NULL) {
        wipe_associated_thumbnails(filepath, config, stats);
    }

        /* 1️⃣ Random rename to obscure filename */
        char rand_name[33];
        const char hex_chars[] = "0123456789abcdef";
        int rnd_fd = open("/dev/urandom", O_RDONLY);
        if (rnd_fd >= 0) {
            uint8_t rnd[16];
            if (read(rnd_fd, rnd, sizeof(rnd)) == sizeof(rnd)) {
                for (int i = 0; i < 16; ++i) {
                    rand_name[i*2] = hex_chars[(rnd[i] >> 4) & 0xF];
                    rand_name[i*2+1] = hex_chars[rnd[i] & 0xF];
                }
                rand_name[32] = '\0';
            } else {
                snprintf(rand_name, sizeof(rand_name), "shred_%d", getpid());
            }
            close(rnd_fd);
        } else {
            snprintf(rand_name, sizeof(rand_name), "shred_%d", getpid());
        }

        char new_path[PATH_MAX];
        char dir_copy[PATH_MAX];
        snprintf(dir_copy, sizeof(dir_copy), "%s", filepath);
        char *dir_name = dirname(dir_copy);
        snprintf(new_path, sizeof(new_path), "%s/%s", dir_name, rand_name);
        if (rename(filepath, new_path) != 0) {
            fprintf(stderr, "\033[1;31m[-] Rename failed for %s: %s\033[0m\n", filepath, strerror(errno));
            stats->errors++;
        } else {
            filepath = new_path;
        }

        /* 2️⃣ Zero timestamps (access & modification) */
        struct timespec zero_ts[2] = {{0,0},{0,0}};
        if (utimensat(AT_FDCWD, filepath, zero_ts, 0) != 0) {
            fprintf(stderr, "\033[1;31m[-] utimensat failed for %s: %s\033[0m\n", filepath, strerror(errno));
            stats->errors++;
        }

        /* 3️⃣ Unlink the file */
        if (unlink(filepath) != 0) {
            fprintf(stderr, "\033[1;31m[-] Failed to unlink %s: %s\033[0m\n", filepath, strerror(errno));
            stats->errors++;
            return -1;
        }

        /* 4️⃣ Sync parent directory to flush journal & signal FTL hardware TRIM */
        char parent_path[PATH_MAX];
        snprintf(parent_path, sizeof(parent_path), "%s", filepath);
        char *parent_dir = dirname(parent_path);
        int dir_fd = open(parent_dir, O_DIRECTORY | O_RDONLY);
        if (dir_fd >= 0) {
            /* Force filesystem journal to commit deletion transaction */
            fsync(dir_fd);

            /* 5️⃣ SSD / NVMe Hardware TRIM Intercept:
             * Signal Flash Translation Layer (FTL) to discard and unmap newly freed blocks.
             * (Gracefully ignored if device is rotational or non-privileged).
             */
#ifdef FITRIM
            struct fstrim_range trim_range;
            memset(&trim_range, 0, sizeof(trim_range));
            trim_range.start = 0;
            trim_range.len = ULLONG_MAX;
            trim_range.minlen = 0;
            ioctl(dir_fd, FITRIM, &trim_range);
#endif
            close(dir_fd);
        } else {
            fprintf(stderr, "\033[1;31m[-] Failed to open parent dir %s: %s\033[0m\n", parent_dir, strerror(errno));
            stats->errors++;
        }

        /* ----- End of chain‑of‑custody steps ----- */

        stats->files_shredded++;
        if (config->verbose) {
            printf("\033[1;32m[+] Shredded (%lu B, %d pass): %s\033[0m\n", (unsigned long)size, config->passes, filepath);
        }

        return 0;
}
