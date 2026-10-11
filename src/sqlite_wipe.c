#define _POSIX_C_SOURCE 200809L

#include "sqlite_wipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>

int sanitize_sqlite_db(const char *db_path, const ShredderConfig *config, SanitizeStats *stats) {
    if (!db_path || !config || !stats) return -1;

    if (access(db_path, F_OK) != 0) {
        return -1;
    }

    if (config->verbose) {
        printf("\033[1;36m[*] Sanitizing SQLite DB: %s\033[0m\n", db_path);
    }

    if (config->dry_run) {
        printf("\033[1;33m[DRY-RUN] Would clear tables, VACUUM, and shred DB: %s\033[0m\n", db_path);
        stats->dbs_sanitized++;
        return 0;
    }

    sqlite3 *db = NULL;
    int rc = sqlite3_open(db_path, &db);
    if (rc == SQLITE_OK && db) {
        sqlite3_stmt *stmt = NULL;
        const char *query = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';";
        
        if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char *table_name = sqlite3_column_text(stmt, 0);
                if (table_name) {
                    char delete_sql[512];
                    snprintf(delete_sql, sizeof(delete_sql), "DELETE FROM \"%s\";", table_name);
                    sqlite3_exec(db, delete_sql, NULL, NULL, NULL);
                }
            }
            sqlite3_finalize(stmt);
        }

        /* VACUUM cleans freelists and overwrites vacated blocks */
        sqlite3_exec(db, "VACUUM;", NULL, NULL, NULL);
        sqlite3_close(db);
        stats->dbs_sanitized++;
    } else {
        if (db) sqlite3_close(db);
        fprintf(stderr, "\033[1;33m[!] Warning: SQLite open failed on %s (proceeding to raw file shred)\033[0m\n", db_path);
    }

    /* Shred companion WAL and journal files if present */
    char aux_path[1024];
    const char *suffixes[] = {"-wal", "-shm", "-journal"};
    for (int i = 0; i < 3; i++) {
        snprintf(aux_path, sizeof(aux_path), "%s%s", db_path, suffixes[i]);
        if (access(aux_path, F_OK) == 0) {
            shred_file(aux_path, config, stats);
        }
    }

    /* Finally shred the main database file */
    return shred_file(db_path, config, stats);
}
