#include "gs_hash.h"
#include <string.h>

// FNV-1a 64-bit
#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

void gs_hash_key_to_coords(const char *key, uint16_t *x, uint16_t *y,
                           uint16_t *z, uint16_t *t) {
  if (!key) {
    *x = *y = *z = *t = 0;
    return;
  }

  uint64_t hash = FNV_OFFSET;
  const char *p = key;

  while (*p) {
    hash ^= (uint64_t)(unsigned char)(*p++);
    hash *= FNV_PRIME;
  }

  // Split 64-bit hash into 4 chunks of 16-bits
  *x = (uint16_t)((hash >> 48) & 0xFFFF);
  *y = (uint16_t)((hash >> 32) & 0xFFFF);
  *z = (uint16_t)((hash >> 16) & 0xFFFF);
  *t = (uint16_t)((hash) & 0xFFFF);
}
