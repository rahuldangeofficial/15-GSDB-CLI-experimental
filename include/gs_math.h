#ifndef GS_MATH_H
#define GS_MATH_H

#include "gs_format.h"
#include <stdint.h>

/**
 * @brief GS Math Library
 * Core logic for "Braiding" 4D coordinates into 1D linear space.
 */

/**
 * @brief Encodes a 4D point into a 64-bit Morton Key (The Braid).
 *
 * Concept:
 * Interleaves bits from x, y, z, t.
 * bit 0 of result = bit 0 of x
 * bit 1 of result = bit 0 of y
 * bit 2 of result = bit 0 of z
 * bit 3 of result = bit 0 of t
 * bit 4 of result = bit 1 of x
 * ...
 *
 * @param x X coordinate (up to 16 bits)
 * @param y Y coordinate (up to 16 bits)
 * @param z Z coordinate (up to 16 bits)
 * @param t T coordinate (up to 16 bits)
 * @return uint64_t The 64-bit Morton Key (Slot ID)
 */
uint64_t gs_encode(uint16_t x, uint16_t y, uint16_t z, uint16_t t);

/**
 * @brief Decodes a 64-bit Morton Key back into a 4D point (The Unbraid).
 *
 * @param morton The 64-bit key
 * @param p Pointer to a gs_point_t structure to fill
 */
void gs_decode(uint64_t morton, gs_point_t *p);

#endif // GS_MATH_H
