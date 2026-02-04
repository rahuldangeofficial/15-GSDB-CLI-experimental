#ifndef GS_INDEX_H
#define GS_INDEX_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief Opaque Index Handler
 */
typedef struct gs_index_s gs_index_t;

/**
 * @brief Creates a new in-memory sparse index
 */
struct gs_index_s *gs_index_create();

/**
 * @brief Frees the index memory
 */
void gs_index_destroy(struct gs_index_s *idx);

/**
 * @brief Looks up a Page ID to find the file offset.
 * @return Offset in file, or 0 if empty (Vacuum).
 */
uint64_t gs_index_lookup(struct gs_index_s *idx, uint64_t page_id);

/**
 * @brief Updates the index with a new Page/Offset mapping.
 */
void gs_index_set(struct gs_index_s *idx, uint64_t page_id, uint64_t offset);

/**
 * @brief Serialize index to file stream
 */
void gs_index_serialize(struct gs_index_s *idx, FILE *f);

/**
 * @brief Deserialize index from file stream
 */
struct gs_index_s *gs_index_deserialize(FILE *f);

/**
 * @brief Get total number of active pages in the index
 */
size_t gs_index_count(struct gs_index_s *idx);

/**
 * @brief Get mapping at specific position (for iteration)
 */
void gs_index_get_at(struct gs_index_s *idx, size_t pos, uint64_t *out_page_id,
                     uint64_t *out_offset);

#endif // GS_INDEX_H
