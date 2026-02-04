#ifndef GS_CORE_H
#define GS_CORE_H

#include <stdint.h>

typedef struct gs_context_s gs_context_t;

/**
 * @brief Initialize the GS Engine
 * @param filename File to open/create
 * @param mode "w" for create/overwrite, "r" for read/update
 */
gs_context_t *gs_open(const char *filename, const char *mode);

/**
 * @brief Close the engine and flush index.
 */
void gs_close(gs_context_t *ctx);

/**
 * @brief Write a value to a 4D coordinate.
 * Automatically handles page creation (Vacuum Folding).
 */
int gs_write(gs_context_t *ctx, uint16_t x, uint16_t y, uint16_t z, uint16_t t,
             uint32_t value);

/**
 * @brief Read a value from a 4D coordinate.
 * Returns 0 if space is empty (no disk access needed).
 */
int gs_read(gs_context_t *ctx, uint16_t x, uint16_t y, uint16_t z, uint16_t t,
            uint32_t *out_value);

/**
 * @brief Iterator function signature
 */
typedef void (*gs_iter_fn)(uint16_t x, uint16_t y, uint16_t z, uint16_t t,
                           uint32_t val, void *user_data);

/**
 * @brief Iterate over all non-zero values in the database
 */
void gs_for_each(gs_context_t *ctx, gs_iter_fn fn, void *user_data);

#endif // GS_CORE_H
