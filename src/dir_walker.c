#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 200809L

#include "dir_walker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ftw.h>

static const ShredderConfig *g_config = NULL;
static SanitizeStats *g_stats = NULL;

static int nftw_callback(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    (void)sb;
    (void)ftwbuf;

    if (tflag == FTW_F) {
        shred_file(fpath, g_config, g_stats);
    } else if (tflag == FTW_DP) {
        if (!g_config->dry_run) {
            if (rmdir(fpath) == 0 && g_config->verbose) {
                printf("\033[1;32m[+] Removed directory: %s\033[0m\n", fpath);
            }
        }
    }
    return 0;
}

int shred_directory(const char *dir_path, const ShredderConfig *config, SanitizeStats *stats) {
    if (!dir_path || !config || !stats) return -1;

    g_config = config;
    g_stats = stats;

    if (config->verbose) {
        printf("\033[1;36m[*] Walking directory tree: %s\033[0m\n", dir_path);
    }

    return nftw(dir_path, nftw_callback, 64, FTW_DEPTH | FTW_PHYS);
}
