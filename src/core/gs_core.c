#include "gs_core.h"
#include "gs_format.h"
#include "gs_index.h"
#include "gs_math.h"
#include "gs_safe.h"
#include <stdio.h>
#include <string.h>

struct gs_context_s {
  FILE *file;
  gs_header_t header;
  struct gs_index_s *index;
};

gs_context_t *gs_open(const char *filename, const char *mode) {
  gs_context_t *ctx = GS_MALLOC(sizeof(gs_context_t));
  if (!ctx)
    return NULL;

  ctx->index = gs_index_create();

  if (strcmp(mode, "w") == 0) {
    ctx->file = fopen(filename, "wb+");
    if (!ctx->file) {
      gs_index_destroy(ctx->index);
      GS_FREE(ctx);
      return NULL;
    }
    gs_header_init(&ctx->header);
    if (fwrite(&ctx->header, sizeof(gs_header_t), 1, ctx->file) != 1) {
      perror("fwrite header");
      fclose(ctx->file);
      gs_index_destroy(ctx->index);
      GS_FREE(ctx);
      return NULL;
    }
  } else {
    ctx->file = fopen(filename, "rb+");
    if (!ctx->file) {
      gs_index_destroy(ctx->index);
      GS_FREE(ctx);
      return NULL;
    }
    if (fread(&ctx->header, sizeof(gs_header_t), 1, ctx->file) != 1) {
      perror("fread header");
      fclose(ctx->file);
      gs_index_destroy(ctx->index);
      GS_FREE(ctx);
      return NULL;
    }
    if (!gs_header_validate(&ctx->header)) {
      fprintf(stderr, "[GSDB] Invalid header magic/version\n");
      fclose(ctx->file);
      gs_index_destroy(ctx->index);
      GS_FREE(ctx);
      return NULL;
    }
    if (ctx->header.index_offset > 0) {
      if (fseek(ctx->file, (long)ctx->header.index_offset, SEEK_SET) == 0) {
        gs_index_destroy(ctx->index);
        ctx->index = gs_index_deserialize(ctx->file);
      }
    }
  }
  return ctx;
}

void gs_close(gs_context_t *ctx) {
  if (ctx) {
    if (ctx->file) {
      if (fseek(ctx->file, 0, SEEK_END) == 0) {
        uint64_t index_pos = (uint64_t)ftell(ctx->file);
        gs_index_serialize(ctx->index, ctx->file);
        ctx->header.index_offset = index_pos;
        if (fseek(ctx->file, 0, SEEK_SET) == 0) {
          if (fwrite(&ctx->header, sizeof(gs_header_t), 1, ctx->file) != 1) {
            perror("fwrite update header");
          }
        }
      }
      fclose(ctx->file);
    }
    if (ctx->index)
      gs_index_destroy(ctx->index);
    GS_FREE(ctx);
  }
}

uint64_t gs_write_page(gs_context_t *ctx, gs_page_header_t *header, void *body,
                       size_t body_size) {
  if (fseek(ctx->file, 0, SEEK_END) != 0)
    return 0;
  uint64_t offset = (uint64_t)ftell(ctx->file);
  if (fwrite(header, sizeof(gs_page_header_t), 1, ctx->file) != 1) {
    perror("fwrite page header");
  }
  if (fwrite(body, body_size, 1, ctx->file) != 1) {
    perror("fwrite page body");
  }
  return offset;
}

int gs_write(gs_context_t *ctx, uint16_t x, uint16_t y, uint16_t z, uint16_t t,
             uint32_t value) {
  uint64_t key = gs_encode(x, y, z, t);
  uint64_t page_id = key / GS_POINTS_PER_PAGE;
  uint16_t offset_in_page = (uint16_t)(key % GS_POINTS_PER_PAGE);

  uint64_t file_offset = gs_index_lookup(ctx->index, page_id);
  gs_page_header_t header;
  header.type = GS_PAGE_TYPE_SPARSE;
  header.count = 0;

  gs_sparse_entry_t sparse_entries[GS_POINTS_PER_PAGE];
  uint32_t dense_data[GS_POINTS_PER_PAGE];
  int is_dense = 0;

  if (file_offset == 0) {
    header.count = 1;
    sparse_entries[0].offset_in_page = offset_in_page;
    sparse_entries[0].value = value;
  } else {
    if (fseek(ctx->file, (long)file_offset, SEEK_SET) != 0)
      return 0;
    if (fread(&header, sizeof(gs_page_header_t), 1, ctx->file) != 1)
      return 0;

    if (header.type == GS_PAGE_TYPE_SPARSE) {
      if (fread(sparse_entries, sizeof(gs_sparse_entry_t), header.count,
                ctx->file) != header.count)
        return 0;
      int found = 0;
      for (int i = 0; i < header.count; i++) {
        if (sparse_entries[i].offset_in_page == offset_in_page) {
          sparse_entries[i].value = value;
          found = 1;
          break;
        }
      }
      if (!found && header.count < GS_POINTS_PER_PAGE) {
        sparse_entries[header.count].offset_in_page = offset_in_page;
        sparse_entries[header.count].value = value;
        header.count++;
      }
      if (header.count > 680) {
        is_dense = 1;
        memset(dense_data, 0, sizeof(dense_data));
        for (int i = 0; i < header.count; i++)
          dense_data[sparse_entries[i].offset_in_page] =
              sparse_entries[i].value;
        header.type = GS_PAGE_TYPE_DENSE;
      }
    } else {
      is_dense = 1;
      if (fread(dense_data, sizeof(uint32_t), GS_POINTS_PER_PAGE, ctx->file) !=
          GS_POINTS_PER_PAGE)
        return 0;
      dense_data[offset_in_page] = value;
    }
  }

  uint64_t new_offset =
      is_dense
          ? gs_write_page(ctx, &header, dense_data, sizeof(dense_data))
          : gs_write_page(ctx, &header, sparse_entries,
                          (size_t)header.count * sizeof(gs_sparse_entry_t));

  gs_index_set(ctx->index, page_id, new_offset);
  return 1;
}

int gs_read(gs_context_t *ctx, uint16_t x, uint16_t y, uint16_t z, uint16_t t,
            uint32_t *out_value) {
  uint64_t key = gs_encode(x, y, z, t);
  uint64_t page_id = key / GS_POINTS_PER_PAGE;
  uint16_t offset_in_page = (uint16_t)(key % GS_POINTS_PER_PAGE);

  uint64_t file_offset = gs_index_lookup(ctx->index, page_id);
  if (file_offset == 0) {
    *out_value = 0;
    return 1;
  }

  gs_page_header_t header;
  if (fseek(ctx->file, (long)file_offset, SEEK_SET) != 0)
    return 0;
  if (fread(&header, sizeof(gs_page_header_t), 1, ctx->file) != 1)
    return 0;

  if (header.type == GS_PAGE_TYPE_SPARSE) {
    gs_sparse_entry_t entries[GS_POINTS_PER_PAGE];
    if (fread(entries, sizeof(gs_sparse_entry_t), header.count, ctx->file) !=
        header.count)
      return 0;
    for (int i = 0; i < header.count; i++) {
      if (entries[i].offset_in_page == offset_in_page) {
        *out_value = entries[i].value;
        return 1;
      }
    }
    *out_value = 0;
  } else {
    if (fseek(ctx->file,
              (long)(file_offset + sizeof(gs_page_header_t) +
                     (offset_in_page * sizeof(uint32_t))),
              SEEK_SET) != 0)
      return 0;
    if (fread(out_value, sizeof(uint32_t), 1, ctx->file) != 1)
      return 0;
  }
  return 1;
}

void gs_for_each(gs_context_t *ctx, gs_iter_fn fn, void *user_data) {
  if (!ctx || !fn)
    return;
  size_t page_count = gs_index_count(ctx->index);
  for (size_t i = 0; i < page_count; i++) {
    uint64_t page_id = 0, file_offset = 0;
    gs_index_get_at(ctx->index, i, &page_id, &file_offset);
    if (file_offset == 0)
      continue;
    if (fseek(ctx->file, (long)file_offset, SEEK_SET) != 0)
      continue;
    gs_page_header_t header;
    if (fread(&header, sizeof(gs_page_header_t), 1, ctx->file) != 1)
      continue;
    if (header.type == GS_PAGE_TYPE_SPARSE) {
      gs_sparse_entry_t entries[GS_POINTS_PER_PAGE];
      if (fread(entries, sizeof(gs_sparse_entry_t), header.count, ctx->file) ==
          header.count) {
        for (int j = 0; j < header.count; j++) {
          gs_point_t p;
          gs_decode((page_id * GS_POINTS_PER_PAGE) + entries[j].offset_in_page,
                    &p);
          fn(p.x, p.y, p.z, p.t, entries[j].value, user_data);
        }
      }
    } else {
      uint32_t data[GS_POINTS_PER_PAGE];
      if (fread(data, sizeof(uint32_t), GS_POINTS_PER_PAGE, ctx->file) ==
          GS_POINTS_PER_PAGE) {
        for (int j = 0; j < GS_POINTS_PER_PAGE; j++) {
          if (data[j] != 0) {
            gs_point_t p;
            gs_decode((page_id * GS_POINTS_PER_PAGE) + j, &p);
            fn(p.x, p.y, p.z, p.t, data[j], user_data);
          }
        }
      }
    }
  }
}
