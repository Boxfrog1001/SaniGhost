#ifndef SQLITE_WIPE_H
#define SQLITE_WIPE_H

#include "shredder.h"

/**
 * Sanitizes SQLite database:
 * 1. Executes DELETE on all user tables.
 * 2. Runs VACUUM to zero out freelist pages.
 * 3. Overwrites and removes database file and related journal/wal/shm files.
 */
int sanitize_sqlite_db(const char *db_path, const ShredderConfig *config, SanitizeStats *stats);

#endif /* SQLITE_WIPE_H */
