#ifndef FTL_PRESSURE_FLUSH_H
#define FTL_PRESSURE_FLUSH_H

#include <stddef.h>
#include <stdint.h>

/**
 * Hardware Topology & FTL Verification Flags
 */
typedef struct {
    int supports_trim;             /* 1 if underlying block device supports TRIM/discard */
    int deterministic_zero;        /* 1 if device guarantees DZAT (discard_zeroes_data == 1) */
    char block_dev_path[128];      /* e.g. /dev/nvme0n1 or /dev/sda */
} BlockDevCapabilities;

/**
 * Query Linux sysfs (/sys/block/<dev>/queue/...) to detect hardware
 * discard behavior and DZAT (Deterministic Read Zero After TRIM).
 */
int query_block_device_caps(const char *mount_or_file_path, BlockDevCapabilities *caps);

/**
 * Free-Space Pressure Flush:
 * Forces SSD controller to execute background garbage collection on stale NAND blocks
 * by generating controlled allocation waves down to target reserve threshold.
 */
int execute_ftl_pressure_flush(const char *target_mount_point, uint64_t target_pressure_mb);

#endif /* FTL_PRESSURE_FLUSH_H */
