#include "gs_format.h"
#include <string.h>

// =========================================================
// CRC32 Implementation (Polynomial 0xEDB88320)
// =========================================================
static uint32_t gs_crc32(const void *data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  const uint8_t *p = (const uint8_t *)data;

  for (size_t i = 0; i < length; i++) {
    crc ^= p[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
    }
  }
  return ~crc;
}

// =========================================================
// Public Format API
// =========================================================

void gs_header_init(gs_header_t *header) {
  memset(header, 0, sizeof(gs_header_t));
  header->magic = GS_MAGIC;
  header->version = GS_VERSION;
  header->page_size_pow = 12; // Default 4KB (2^12)
  header->total_points = 0;
  header->index_offset = sizeof(gs_header_t); // Index starts right after header
  header->data_offset = 0;                    // Set when data is written
  header->crc32 = 0;
}

int gs_header_validate(const gs_header_t *header) {
  if (header->magic != GS_MAGIC)
    return 0;
  if (header->version != GS_VERSION)
    return 0;
  return 1;
}

uint32_t gs_compute_checksum(const void *data, size_t len) {
  return gs_crc32(data, len);
}
