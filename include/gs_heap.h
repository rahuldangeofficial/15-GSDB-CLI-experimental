#ifndef GS_HEAP_H
#define GS_HEAP_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque Heap Handle
 * Manages two files:
 * 1. .heap (Data)
 * 2. .hidx (Index: ID -> Offset)
 */
typedef struct gs_heap_s gs_heap_t;

/**
 * @brief Open a Heap Storage context
 * @param filename Base filename (e.g. "main"). Will append .heap and .hidx
 * @return Handle or NULL
 */
gs_heap_t *gs_heap_open(const char *base_filename);

/**
 * @brief Close the Heap
 */
void gs_heap_close(gs_heap_t *heap);

/**
 * @brief Append an arbitrary binary blob to the heap
 * @param heap Handle
 * @param data Pointer to data
 * @param size Size in bytes
 * @return The new Heap ID (uint32_t) to be stored in the DB
 */
uint32_t gs_heap_append(gs_heap_t *heap, const void *data, size_t size);

/**
 * @brief Retrieve a blob from the heap
 * @param heap Handle
 * @param id Heap ID
 * @param out_data [Output] Pointer to buffer (will be malloc'd, user must free)
 * @param out_size [Output] Size of data
 * @return 1 on success, 0 on failure
 */
int gs_heap_get(gs_heap_t *heap, uint32_t id, void **out_data,
                size_t *out_size);

#endif // GS_HEAP_H
