#include "gs_safe.h"
#include <stdio.h>
#include <stdlib.h>

static int g_alloc_count = 0;

void *gs_malloc_tracked(size_t size, const char *file, int line) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "[GS-SAFE] FATAL: Out of memory at %s:%d\n", file, line);
    // In a real lib, we might return NULL or abort.
    // For MVP stability, we log error.
    return NULL;
  }
  g_alloc_count++;
  return ptr;
}

void gs_free_tracked(void *ptr) {
  if (ptr) {
    g_alloc_count--;
    free(ptr);
  }
}

int gs_get_allocation_count(void) { return g_alloc_count; }
