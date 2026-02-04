#ifndef GS_FORMAT_H
#define GS_FORMAT_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Geometric Storage File Format Standard
 *
 * This file defines the on-disk structure of a .gs file.
 * It is designed to be:
 * 1. Minimal: No bloat.
 * 2. Aligned: Optimized for 64-bit word access.
 * 3. Verified: Magic numbers and versioning.
 */

// Magic Number: "GSDB" in ASCII (0x47534442)
// Identifies the file as a Geometric Storage Database.
#define GS_MAGIC 0x47534442

// Current Version of the layout
#define GS_VERSION 1

// The Vacuum Limit: We aim for 10-15% maximum overhead.
#define GS_MAX_OVERHEAD_PERCENT 15

/**
 * @brief The Master Header
 * strictly packed to ensure identical layout on all compilers.
 */
typedef struct __attribute__((packed)) {
  uint32_t magic;         // 0x00: GS_MAGIC
  uint16_t version;       // 0x04: GS_VERSION
  uint16_t page_size_pow; // 0x06: Power of 2 for page size (e.g., 12 for 4KB)
  uint64_t total_points;  // 0x08: Total data points stored
  uint64_t index_offset;  // 0x10: File offset to the Sparse Index
  uint64_t data_offset;   // 0x18: File offset to the Data Blocks
  uint64_t crc32;         // 0x20: Checksum for integrity
                          // Total Size: 40 bytes
} gs_header_t;

/**
 * @brief A Single 4D Coordinate Point (Verification Structure)
 * Used for testing the math, not necessarily how it's stored on disk (which is
 * compressed).
 */
typedef struct {
  uint16_t x;
  uint16_t y;
  uint16_t z;
  uint16_t t;
} gs_point_t;

// =========================================================
// Adaptive Page Format
// =========================================================

#define GS_PAGE_TYPE_SPARSE 0x00
#define GS_PAGE_TYPE_DENSE 0x01

// Maximum points per page (4096 / 4 bytes = 1024)
#define GS_POINTS_PER_PAGE 1024

// Sparse Entry: 2 bytes offset + 4 bytes value = 6 bytes overhead
// Compared to 4 bytes raw data.
// Total overhead for 1 point: Header(3B) + Entry(6B) = 9 bytes vs 4KB Page.
typedef struct {
  uint16_t offset_in_page; // 0..1023
  uint32_t value;
} __attribute__((packed)) gs_sparse_entry_t;

// Page Header (3 Bytes)
typedef struct {
  uint8_t type;   // SPARSE or DENSE
  uint16_t count; // Number of points in page
} __attribute__((packed)) gs_page_header_t;

// =========================================================
// Format Utilities
// =========================================================

/**
 * @brief Initialize a header with default values and Magic Number.
 */
void gs_header_init(gs_header_t *header);

/**
 * @brief Validate a header's Magic Number and Version.
 * @return 1 if valid, 0 if invalid.
 */
int gs_header_validate(const gs_header_t *header);

/**
 * @brief Compute CRC32 checksum for data.
 */
uint32_t gs_compute_checksum(const void *data, size_t len);

#endif // GS_FORMAT_H
