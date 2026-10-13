#ifndef DIRECT_IO_BYPASS_H
#define DIRECT_IO_BYPASS_H

#include <stddef.h>

/**
 * Open file descriptor with direct Linux page-cache bypass (O_DIRECT | O_SYNC).
 * Requires sector-aligned buffers (typically 4096-byte boundaries) for read/write.
 */
int open_direct_io_target(const char *path);

/**
 * Allocate aligned memory buffer suitable for O_DIRECT DMA transfers.
 */
void *alloc_aligned_block_buffer(size_t size, size_t alignment);

/**
 * Free aligned block buffer.
 */
void free_aligned_block_buffer(void *ptr);

#endif /* DIRECT_IO_BYPASS_H */
