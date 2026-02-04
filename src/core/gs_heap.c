#include "gs_heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct gs_heap_s {
  FILE *data_file;  // .heap
  FILE *index_file; // .hidx
  uint32_t count;
};

gs_heap_t *gs_heap_open(const char *base_filename) {
  if (!base_filename)
    return NULL;

  gs_heap_t *heap = malloc(sizeof(gs_heap_t));
  if (!heap)
    return NULL;

  char path_data[256];
  char path_idx[256];
  snprintf(path_data, sizeof(path_data), "%s.heap", base_filename);
  snprintf(path_idx, sizeof(path_idx), "%s.hidx", base_filename);

  heap->data_file = fopen(path_data, "a+b");
  if (!heap->data_file) {
    free(heap);
    return NULL;
  }

  heap->index_file = fopen(path_idx, "a+b");
  if (!heap->index_file) {
    fclose(heap->data_file);
    free(heap);
    return NULL;
  }

  if (fseek(heap->index_file, 0, SEEK_END) == 0) {
    long idx_size = ftell(heap->index_file);
    if (idx_size >= 0) {
      heap->count = (uint32_t)((size_t)idx_size / sizeof(uint64_t));
    } else {
      heap->count = 0;
    }
  } else {
    heap->count = 0;
  }

  return heap;
}

void gs_heap_close(gs_heap_t *heap) {
  if (heap) {
    if (heap->data_file)
      fclose(heap->data_file);
    if (heap->index_file)
      fclose(heap->index_file);
    free(heap);
  }
}

uint32_t gs_heap_append(gs_heap_t *heap, const void *data, size_t size) {
  if (!heap || !data)
    return 0xFFFFFFFF;

  if (fseek(heap->data_file, 0, SEEK_END) != 0)
    return 0xFFFFFFFF;
  uint64_t offset = (uint64_t)ftell(heap->data_file);

  uint64_t size64 = (uint64_t)size;
  if (fwrite(&size64, sizeof(uint64_t), 1, heap->data_file) != 1) {
    perror("fwrite heap size");
    return 0xFFFFFFFF;
  }
  if (fwrite(data, 1, size, heap->data_file) != size) {
    perror("fwrite heap data");
    return 0xFFFFFFFF;
  }
  if (fflush(heap->data_file) != 0) {
    perror("fflush heap data");
  }

  if (fseek(heap->index_file, 0, SEEK_END) == 0) {
    if (fwrite(&offset, sizeof(uint64_t), 1, heap->index_file) != 1) {
      perror("fwrite heap index");
    }
  }
  if (fflush(heap->index_file) != 0) {
    perror("fflush heap index");
  }

  uint32_t id = heap->count + 1;
  heap->count++;

  return id;
}

int gs_heap_get(gs_heap_t *heap, uint32_t id, void **out_data,
                size_t *out_size) {
  if (!heap || !out_data || !out_size)
    return 0;
  if (id == 0)
    return 0;

  uint32_t real_id = id - 1;
  if (real_id >= heap->count)
    return 0;

  uint64_t offset = 0;
  if (fseek(heap->index_file, (long)((size_t)real_id * sizeof(uint64_t)),
            SEEK_SET) != 0)
    return 0;
  if (fread(&offset, sizeof(uint64_t), 1, heap->index_file) != 1)
    return 0;

  if (fseek(heap->data_file, (long)offset, SEEK_SET) != 0)
    return 0;

  uint64_t size64 = 0;
  if (fread(&size64, sizeof(uint64_t), 1, heap->data_file) != 1)
    return 0;

  void *buffer = malloc((size_t)size64);
  if (!buffer)
    return 0;

  if (fread(buffer, 1, (size_t)size64, heap->data_file) != (size_t)size64) {
    perror("fread heap data");
    free(buffer);
    return 0;
  }

  *out_data = buffer;
  *out_size = (size_t)size64;
  return 1;
}
