#include "gs_index.h"
#include "gs_safe.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint64_t page_id;
  uint64_t file_offset;
} gs_index_entry_t;

typedef struct gs_index_s {
  gs_index_entry_t *entries;
  size_t count;
  size_t capacity;
} gs_index_t;

gs_index_t *gs_index_create() {
  gs_index_t *idx = GS_MALLOC(sizeof(gs_index_t));
  if (!idx)
    return NULL;
  idx->count = 0;
  idx->capacity = 16;
  idx->entries = GS_MALLOC(sizeof(gs_index_entry_t) * idx->capacity);
  return idx;
}

void gs_index_destroy(gs_index_t *idx) {
  if (idx) {
    if (idx->entries)
      GS_FREE(idx->entries);
    GS_FREE(idx);
  }
}

uint64_t gs_index_lookup(gs_index_t *idx, uint64_t page_id) {
  if (!idx || idx->count == 0)
    return 0;

  size_t left = 0;
  size_t right = idx->count;

  while (left < right) {
    size_t mid = left + (right - left) / 2;
    if (idx->entries[mid].page_id == page_id) {
      return idx->entries[mid].file_offset;
    } else if (idx->entries[mid].page_id < page_id) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return 0;
}

void gs_index_set(gs_index_t *idx, uint64_t page_id, uint64_t offset) {
  if (!idx)
    return;

  size_t left = 0;
  size_t right = idx->count;
  size_t insert_pos = idx->count;

  while (left < right) {
    size_t mid = left + (right - left) / 2;
    if (idx->entries[mid].page_id == page_id) {
      idx->entries[mid].file_offset = offset;
      return;
    } else if (idx->entries[mid].page_id < page_id) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  insert_pos = left;

  if (idx->count >= idx->capacity) {
    size_t new_cap = idx->capacity * 2;
    gs_index_entry_t *new_entries =
        GS_MALLOC(sizeof(gs_index_entry_t) * new_cap);
    if (!new_entries)
      return;
    memcpy(new_entries, idx->entries, sizeof(gs_index_entry_t) * idx->count);
    GS_FREE(idx->entries);
    idx->entries = new_entries;
    idx->capacity = new_cap;
  }

  if (insert_pos < idx->count) {
    memmove(&idx->entries[insert_pos + 1], &idx->entries[insert_pos],
            sizeof(gs_index_entry_t) * (idx->count - insert_pos));
  }

  idx->entries[insert_pos].page_id = page_id;
  idx->entries[insert_pos].file_offset = offset;
  idx->count++;
}

void gs_index_serialize(gs_index_t *idx, FILE *f) {
  if (!idx || !f)
    return;
  (void)fwrite(&idx->count, sizeof(size_t), 1, f);
  if (idx->count > 0) {
    (void)fwrite(idx->entries, sizeof(gs_index_entry_t), idx->count, f);
  }
}

gs_index_t *gs_index_deserialize(FILE *f) {
  if (!f)
    return NULL;

  gs_index_t *idx = gs_index_create();
  if (!idx)
    return NULL;

  size_t count = 0;
  if (fread(&count, sizeof(size_t), 1, f) != 1) {
    return idx;
  }

  if (count > idx->capacity) {
    GS_FREE(idx->entries);
    idx->capacity = count + 16;
    idx->entries = GS_MALLOC(sizeof(gs_index_entry_t) * idx->capacity);
  }

  idx->count = count;
  if (count > 0) {
    (void)fread(idx->entries, sizeof(gs_index_entry_t), count, f);
  }
  return idx;
}

size_t gs_index_count(gs_index_t *idx) { return idx ? idx->count : 0; }

void gs_index_get_at(gs_index_t *idx, size_t pos, uint64_t *out_page_id,
                     uint64_t *out_offset) {
  if (!idx || pos >= idx->count)
    return;
  if (out_page_id)
    *out_page_id = idx->entries[pos].page_id;
  if (out_offset)
    *out_offset = idx->entries[pos].file_offset;
}
