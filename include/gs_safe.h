#ifndef GS_SAFE_H
#define GS_SAFE_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Thread-Safe Memory Tracker
 * Wraps malloc/free to ensure NO MEMORY LEAKS.
 */

void *gs_malloc_tracked(size_t size, const char *file, int line);
void gs_free_tracked(void *ptr);

/**
 * @brief Returns number of active allocations.
 * Should be 0 at program exit.
 */
int gs_get_allocation_count(void);

#define GS_MALLOC(size) gs_malloc_tracked(size, __FILE__, __LINE__)
#define GS_FREE(ptr) gs_free_tracked(ptr)

#endif // GS_SAFE_H
