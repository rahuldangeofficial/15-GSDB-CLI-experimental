#include "gs_db.h"
#include "gs_core.h"
#include "gs_hash.h"
#include "gs_heap.h"
#include "gs_query.h"
#include "gs_safe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct gs_db_s {
  gs_context_t *ctx;
  gs_heap_t *heap;
};

gsdb_t *gsdb_open(const char *filename) {
  gsdb_t *db = malloc(sizeof(gsdb_t));
  if (!db)
    return NULL;

  db->ctx = gs_open(filename, "r+");
  if (!db->ctx)
    db->ctx = gs_open(filename, "w");

  char base[256] = {0};
  if (filename) {
    strncpy(base, filename, sizeof(base) - 1);
    char *ext = strstr(base, ".gs");
    if (ext)
      *ext = 0;
  } else {
    strcpy(base, "default");
  }

  db->heap = gs_heap_open(base);
  if (!db->ctx || !db->heap) {
    if (db->ctx)
      gs_close(db->ctx);
    if (db->heap)
      gs_heap_close(db->heap);
    free(db);
    return NULL;
  }
  return db;
}

void gsdb_close(gsdb_t *db) {
  if (db) {
    if (db->ctx)
      gs_close(db->ctx);
    if (db->heap)
      gs_heap_close(db->heap);
    free(db);
  }
}

struct vac_ctx {
  gsdb_t *db;
  gs_heap_t *nheap;
};

void vac_fn(uint16_t x, uint16_t y, uint16_t z, uint16_t t, uint32_t val,
            void *u) {
  struct vac_ctx *v = (struct vac_ctx *)u;
  if (val != 0) {
    void *data = NULL;
    size_t size = 0;
    if (gs_heap_get(v->db->heap, val, &data, &size)) {
      uint32_t new_id = gs_heap_append(v->nheap, data, size);
      (void)gs_write(v->db->ctx, x, y, z, t, new_id);
      free(data);
    }
  }
}

struct count_ctx {
  int count;
};

void count_fn(uint16_t x, uint16_t y, uint16_t z, uint16_t t, uint32_t val,
              void *u) {
  (void)x;
  (void)y;
  (void)z;
  (void)t;
  if (val != 0)
    ((struct count_ctx *)u)->count++;
}

gsdb_result_t gsdb_vacuum(gsdb_t *db) {
  gsdb_result_t res;
  memset(&res, 0, sizeof(res));
  printf("[GSDB] Vacuuming... Shadow-copying active heap to 'compact.heap'.\n");
  gs_heap_t *new_heap = gs_heap_open("compact");
  if (!new_heap) {
    res.status = 0;
    strncpy(res.message, "Vacuum Failed: Could not create compact heap.",
            sizeof(res.message) - 1);
    return res;
  }
  struct vac_ctx vctx = {db, new_heap};
  gs_for_each(db->ctx, vac_fn, &vctx);
  gs_heap_close(new_heap);
  res.status = 1;
  strncpy(res.message,
          "VACUUM completed. Space reclaimed (Active IDs re-mapped).",
          sizeof(res.message) - 1);
  return res;
}

gsdb_result_t gsdb_execute(gsdb_t *db, const char *command) {
  gsdb_result_t res;
  memset(&res, 0, sizeof(res));
  if (!db || !db->ctx) {
    res.status = 0;
    strncpy(res.message, "Database not connected", sizeof(res.message) - 1);
    return res;
  }

  gsdb_query_t q;
  if (!gsdb_parse_command(command, &q)) {
    res.status = 0;
    strncpy(res.message, "Syntax Error", sizeof(res.message) - 1);
    return res;
  }

  switch (q.type) {
  case CMD_PUT:
    if (gs_write(db->ctx, q.x, q.y, q.z, q.t, q.val)) {
      res.status = 1;
      snprintf(res.message, sizeof(res.message),
               "OK. Wrote %u to (%u,%u,%u,%u)", q.val, q.x, q.y, q.z, q.t);
    } else {
      res.status = 0;
      strncpy(res.message, "Write Failed", sizeof(res.message) - 1);
    }
    break;
  case CMD_GET: {
    uint32_t val = 0;
    if (gs_read(db->ctx, q.x, q.y, q.z, q.t, &val)) {
      res.status = 1;
      res.val = val;
      if (val == 0)
        snprintf(res.message, sizeof(res.message), "OK. Vacuum (0)");
      else
        snprintf(res.message, sizeof(res.message), "OK. %u", val);
    } else {
      res.status = 0;
      strncpy(res.message, "Read Failed (IO Error)", sizeof(res.message) - 1);
    }
  } break;
  case CMD_IMPORT: {
    FILE *f = fopen(q.arg1, "rb");
    if (!f) {
      res.status = 0;
      snprintf(res.message, sizeof(res.message), "File not found: %s", q.arg1);
      break;
    }
    fseek(f, 0, SEEK_END);
    size_t fsize = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    void *data = malloc(fsize);
    if (!data || fread(data, 1, fsize, f) != fsize) {
      free(data);
      fclose(f);
      res.status = 0;
      strncpy(res.message, "Read Error", sizeof(res.message) - 1);
      break;
    }
    fclose(f);
    uint32_t id = gs_heap_append(db->heap, data, fsize);
    free(data);
    res.status = 1;
    snprintf(res.message, sizeof(res.message), "Imported. HEAP_ID: %u", id);
  } break;
  case CMD_EXPORT: {
    void *data = NULL;
    size_t size = 0;
    if (gs_heap_get(db->heap, q.val, &data, &size)) {
      FILE *f = fopen(q.arg1, "wb");
      if (f) {
        if (fwrite(data, 1, size, f) != size)
          perror("fwrite export");
        fclose(f);
        res.status = 1;
        snprintf(res.message, sizeof(res.message), "Exported %zu bytes to %s",
                 size, q.arg1);
      } else {
        res.status = 0;
        strncpy(res.message, "Write Error", sizeof(res.message) - 1);
      }
      free(data);
    } else {
      res.status = 0;
      strncpy(res.message, "Heap ID Not Found", sizeof(res.message) - 1);
    }
  } break;
  case CMD_INSERT: {
    FILE *f = fopen(q.arg2, "rb");
    if (!f) {
      res.status = 0;
      snprintf(res.message, sizeof(res.message), "File not found: %s", q.arg2);
      break;
    }
    fseek(f, 0, SEEK_END);
    size_t fsize = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    void *data = malloc(fsize);
    if (!data || fread(data, 1, fsize, f) != fsize) {
      free(data);
      fclose(f);
      res.status = 0;
      strncpy(res.message, "Read Error", sizeof(res.message) - 1);
      break;
    }
    fclose(f);
    uint32_t id = gs_heap_append(db->heap, data, fsize);
    free(data);
    uint16_t x, y, z, t;
    gs_hash_key_to_coords(q.arg1, &x, &y, &z, &t);
    if (gs_write(db->ctx, x, y, z, t, id)) {
      res.status = 1;
      snprintf(res.message, sizeof(res.message),
               "Inserted '%s' (ID: %u) at (%u,%u,%u,%u)", q.arg1, id, x, y, z,
               t);
    } else {
      res.status = 0;
      strncpy(res.message, "Index Write Failed", sizeof(res.message) - 1);
    }
  } break;
  case CMD_FIND: {
    uint16_t x, y, z, t;
    gs_hash_key_to_coords(q.arg1, &x, &y, &z, &t);
    uint32_t val = 0;
    if (gs_read(db->ctx, x, y, z, t, &val)) {
      res.status = 1;
      res.val = val;
      if (val == 0)
        snprintf(res.message, sizeof(res.message), "Key '%s' not found (0)",
                 q.arg1);
      else
        snprintf(res.message, sizeof(res.message),
                 "Found '%s': ID %u at (%u,%u,%u,%u)", q.arg1, val, x, y, z, t);
    } else {
      res.status = 0;
      strncpy(res.message, "Read Failed", sizeof(res.message) - 1);
    }
  } break;
  case CMD_MONGO_INSERT: {
    char key[2048] = {0};
    char *json = q.arg1;
    char *colon = strchr(json, ':');
    if (colon) {
      char *val_start = strchr(colon, '"');
      if (val_start) {
        char *val_end = strchr(val_start + 1, '"');
        if (val_end) {
          size_t len = (size_t)(val_end - (val_start + 1));
          if (len > 2047)
            len = 2047;
          strncpy(key, val_start + 1, len);
          key[len] = '\0';
        }
      }
    }
    if (strlen(key) == 0) {
      res.status = 0;
      strncpy(res.message, "No Key Found", sizeof(res.message) - 1);
      break;
    }
    uint32_t id = gs_heap_append(db->heap, json, strlen(json) + 1);
    uint16_t x, y, z, t;
    gs_hash_key_to_coords(key, &x, &y, &z, &t);
    if (gs_write(db->ctx, x, y, z, t, id)) {
      res.status = 1;
      snprintf(res.message, sizeof(res.message),
               "{ \"status\": \"ok\", \"id\": %u, \"key\": \"%s\" }", id, key);
    } else {
      res.status = 0;
      strncpy(res.message,
              "{ \"status\": \"error\", \"msg\": \"Write Failed\" }",
              sizeof(res.message) - 1);
    }
  } break;
  case CMD_MONGO_FIND: {
    char key[2048] = {0};
    char *json = q.arg1;
    char *colon = strchr(json, ':');
    if (colon) {
      char *val_start = strchr(colon, '"');
      if (val_start) {
        char *val_end = strchr(val_start + 1, '"');
        if (val_end) {
          size_t len = (size_t)(val_end - (val_start + 1));
          if (len > 2047)
            len = 2047;
          strncpy(key, val_start + 1, len);
          key[len] = '\0';
        }
      }
    }
    if (strlen(key) == 0) {
      res.status = 0;
      strncpy(res.message, "No Key Found", sizeof(res.message) - 1);
      break;
    }
    uint16_t x, y, z, t;
    gs_hash_key_to_coords(key, &x, &y, &z, &t);
    uint32_t id = 0;
    if (gs_read(db->ctx, x, y, z, t, &id) && id != 0) {
      void *data = NULL;
      size_t size = 0;
      if (gs_heap_get(db->heap, id, &data, &size)) {
        res.status = 1;
        snprintf(res.message, sizeof(res.message), "%s", (char *)data);
        free(data);
      } else {
        res.status = 0;
        strncpy(res.message, "Data Missing", sizeof(res.message) - 1);
      }
    } else {
      res.status = 0;
      strncpy(res.message, "Not Found", sizeof(res.message) - 1);
    }
  } break;
  case CMD_MONGO_DELETE: {
    char key[2048] = {0};
    char *json = q.arg1;
    char *colon = strchr(json, ':');
    if (colon) {
      char *val_start = strchr(colon, '"');
      if (val_start) {
        char *val_end = strchr(val_start + 1, '"');
        if (val_end) {
          size_t len = (size_t)(val_end - (val_start + 1));
          if (len > 2047)
            len = 2047;
          strncpy(key, val_start + 1, len);
          key[len] = '\0';
        }
      }
    }
    if (strlen(key) == 0) {
      res.status = 0;
      strncpy(res.message, "No Key Found", sizeof(res.message) - 1);
      break;
    }
    uint16_t x, y, z, t;
    gs_hash_key_to_coords(key, &x, &y, &z, &t);
    if (gs_write(db->ctx, x, y, z, t, 0)) {
      res.status = 1;
      snprintf(res.message, sizeof(res.message),
               "{ \"status\": \"ok\", \"deleted\": \"%s\" }", key);
    } else {
      res.status = 0;
      strncpy(res.message, "Delete Failed", sizeof(res.message) - 1);
    }
  } break;
  case CMD_MONGO_UPDATE: {
    char key[2048] = {0};
    char *json = q.arg1;
    if (strstr(json, "\"k\":")) {
      const char *k_start = strstr(json, "\"k\":") + 4;
      if (sscanf(k_start, " \"%2047[^\"]\"", key) != 1) {
        perror("sscanf update key");
      }
    }
    uint16_t x, y, z, t;
    gs_hash_key_to_coords(key, &x, &y, &z, &t);
    uint32_t curr = 0;
    if (gs_read(db->ctx, x, y, z, t, &curr) && curr != 0) {
      uint32_t nid = gs_heap_append(db->heap, json, strlen(json) + 1);
      (void)gs_write(db->ctx, x, y, z, t, nid);
      res.status = 1;
      strncpy(res.message, "{ \"status\": \"ok\", \"msg\": \"Updated\" }",
              sizeof(res.message) - 1);
    } else {
      res.status = 0;
      strncpy(res.message, "Key Not Found", sizeof(res.message) - 1);
    }
  } break;
  case CMD_MONGO_COUNT: {
    struct count_ctx cctx = {0};
    gs_for_each(db->ctx, count_fn, &cctx);
    res.status = 1;
    snprintf(res.message, sizeof(res.message),
             "{ \"status\": \"ok\", \"count\": %d }", cctx.count);
  } break;
  case CMD_VACUUM:
    res = gsdb_vacuum(db);
    break;
  case CMD_STATS: {
    res.status = 1;
    int active = gs_get_allocation_count();
    snprintf(res.message, sizeof(res.message), "Allocations Active: %d",
             active);
  } break;
  case CMD_EXIT:
    res.status = 1;
    strncpy(res.message, "BYE", sizeof(res.message) - 1);
    break;
  default:
    res.status = 0;
    strncpy(res.message, "Unknown Command", sizeof(res.message) - 1);
    break;
  }
  return res;
}
