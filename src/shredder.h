#ifndef SHREDDER_H
#define SHREDDER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t files_shredded;
    uint64_t bytes_overwritten;
    uint64_t dbs_sanitized;
    uint64_t errors;
    double start_time;
    double end_time;
} SanitizeStats;

typedef struct {
    int passes;
    bool dry_run;
    bool verbose;
} ShredderConfig;

/**
 * Shreds a single file using unbuffered multi-pass writes followed by ftruncate and unlink.
 * Returns 0 on success, -1 on failure.
 */
int shred_file(const char *filepath, const ShredderConfig *config, SanitizeStats *stats);

#endif /* SHREDDER_H */
