#ifndef DIR_WALKER_H
#define DIR_WALKER_H

#include "shredder.h"

/**
 * Recursively shreds all files in a directory tree and removes the directories.
 */
int shred_directory(const char *dir_path, const ShredderConfig *config, SanitizeStats *stats);

#endif /* DIR_WALKER_H */
