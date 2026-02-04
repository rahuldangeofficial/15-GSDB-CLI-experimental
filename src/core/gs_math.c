#include "gs_math.h"

static uint64_t spread_4(uint16_t x) {
  uint64_t res = 0;
  for (int i = 0; i < 16; i++) {
    if ((x >> i) & 1)
      res |= (1ULL << (size_t)(i * 4));
  }
  return res;
}

static uint16_t compact_4(uint64_t x) {
  uint16_t res = 0;
  for (int i = 0; i < 16; i++) {
    if ((x >> (size_t)(i * 4)) & 1)
      res |= (uint16_t)(1 << i);
  }
  return res;
}

uint64_t gs_encode(uint16_t x, uint16_t y, uint16_t z, uint16_t t) {
  return spread_4(x) | (spread_4(y) << 1) | (spread_4(z) << 2) |
         (spread_4(t) << 3);
}

void gs_decode(uint64_t morton, gs_point_t *p) {
  p->x = compact_4(morton);
  p->y = compact_4(morton >> 1);
  p->z = compact_4(morton >> 2);
  p->t = compact_4(morton >> 3);
}
