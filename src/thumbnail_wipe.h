#ifndef THUMBNAIL_WIPE_H
#define THUMBNAIL_WIPE_H

#include "shredder.h"

/**
 * Checks for and securely obliterates any FreeDesktop cached thumbnails
 * (~/.cache/thumbnails/normal, large, x-large, xx-large, fail) associated
 * with the target file.
 *
 * Implemented per the FreeDesktop Thumbnail Managing Standard v0.8.0.
 * Zero external dependencies; includes native RFC 1321 MD5 URI hasher.
 */
int wipe_associated_thumbnails(const char *filepath, const ShredderConfig *config, SanitizeStats *stats);

#endif /* THUMBNAIL_WIPE_H */
