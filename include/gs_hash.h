#ifndef GS_HASH_H
#define GS_HASH_H

#include <stdint.h>

/**
 * @brief Hash a string key into 4D coordinates (x,y,z,t)
 * Uses FNV-1a 64-bit hash, split into 4x 16-bit components.
 *
 * @param key Null-terminated string key
 * @param x [Output] X coordinate
 * @param y [Output] Y coordinate
 * @param z [Output] Z coordinate
 * @param t [Output] T coordinate
 */
void gs_hash_key_to_coords(const char *key, uint16_t *x, uint16_t *y,
                           uint16_t *z, uint16_t *t);

#endif // GS_HASH_H
