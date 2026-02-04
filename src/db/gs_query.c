#include "gs_query.h"
#include <stdio.h>
#include <string.h>
#include <strings.h> // strncasecmp

int gsdb_parse_command(const char *input, gsdb_query_t *out_query) {
  if (!input || !out_query)
    return 0;

  // Default
  out_query->type = CMD_UNKNOWN;

  if (strncasecmp(input, "PUT", 3) == 0) {
    if (sscanf(input + 3, "%hu %hu %hu %hu %u", &out_query->x, &out_query->y,
               &out_query->z, &out_query->t, &out_query->val) == 5) {
      out_query->type = CMD_PUT;
      return 1;
    }
  } else if (strncasecmp(input, "GET", 3) == 0) {
    if (sscanf(input + 3, "%hu %hu %hu %hu", &out_query->x, &out_query->y,
               &out_query->z, &out_query->t) == 4) {
      out_query->type = CMD_GET;
      return 1;
    }
  } else if (strncasecmp(input, "STATS", 5) == 0) {
    out_query->type = CMD_STATS;
    return 1;
  } else if (strncasecmp(input, "VACUUM", 6) == 0) {
    out_query->type = CMD_VACUUM;
    return 1;
  } else if (strncasecmp(input, "IMPORT", 6) == 0) {
    if (sscanf(input + 6, "%2047s", out_query->arg1) == 1) {
      out_query->type = CMD_IMPORT;
      return 1;
    }
  } else if (strncasecmp(input, "EXPORT", 6) == 0) {
    if (sscanf(input + 6, "%u %2047s", &out_query->val, out_query->arg1) == 2) {
      out_query->type = CMD_EXPORT;
      return 1;
    }
  } else if (strncasecmp(input, "INSERT", 6) == 0) {
    char *start = strchr(input + 6, '"');
    if (start) {
      char *end = strchr(start + 1, '"');
      if (end) {
        size_t len = (size_t)(end - (start + 1));
        if (len > 2047)
          len = 2047;
        strncpy(out_query->arg1, start + 1, len);
        out_query->arg1[len] = '\0';
        if (sscanf(end + 1, "%2047s", out_query->arg2) != 1) {
          /* Optional filename */
        }
        out_query->type = CMD_INSERT;
        return 1;
      }
    }
  } else if (strncasecmp(input, "FIND", 4) == 0) {
    char *start = strchr(input + 4, '"');
    if (start) {
      char *end = strchr(start + 1, '"');
      if (end) {
        size_t len = (size_t)(end - (start + 1));
        if (len > 2047)
          len = 2047;
        strncpy(out_query->arg1, start + 1, len);
        out_query->arg1[len] = '\0';
        out_query->type = CMD_FIND;
        return 1;
      }
    }
  } else if (strncasecmp(input, "FETCH", 5) == 0) {
    char *start = strchr(input + 5, '"');
    if (start) {
      char *end = strchr(start + 1, '"');
      if (end) {
        size_t len = (size_t)(end - (start + 1));
        if (len > 2047)
          len = 2047;
        strncpy(out_query->arg1, start + 1, len);
        out_query->arg1[len] = '\0';
        if (sscanf(end + 1, "%2047s", out_query->arg2) != 1) {
          /* Optional filename */
        }
        out_query->type = CMD_FETCH;
        return 1;
      }
    }
  } else if (strncasecmp(input, "db.insert", 9) == 0) {
    char *start = strchr(input, '{');
    char *end = strrchr(input, '}');
    if (start && end && end > start) {
      size_t len = (size_t)(end - start + 1);
      if (len > 2047)
        len = 2047;
      strncpy(out_query->arg1, start, len);
      out_query->arg1[len] = '\0';
      out_query->type = CMD_MONGO_INSERT;
      return 1;
    }
  } else if (strncasecmp(input, "db.find", 7) == 0) {
    char *start = strchr(input, '{');
    char *end = strrchr(input, '}');
    if (start && end && end > start) {
      size_t len = (size_t)(end - start + 1);
      if (len > 2047)
        len = 2047;
      strncpy(out_query->arg1, start, len);
      out_query->arg1[len] = '\0';
      out_query->type = CMD_MONGO_FIND;
      return 1;
    }
  } else if (strncasecmp(input, "db.delete", 9) == 0) {
    char *start = strchr(input, '{');
    char *end = strrchr(input, '}');
    if (start && end && end > start) {
      size_t len = (size_t)(end - start + 1);
      if (len > 2047)
        len = 2047;
      strncpy(out_query->arg1, start, len);
      out_query->arg1[len] = '\0';
      out_query->type = CMD_MONGO_DELETE;
      return 1;
    }
  } else if (strncasecmp(input, "db.update", 9) == 0) {
    char *start = strchr(input, '{');
    char *end = strrchr(input, '}');
    if (start && end && end > start) {
      size_t len = (size_t)(end - start + 1);
      if (len > 2047)
        len = 2047;
      strncpy(out_query->arg1, start, len);
      out_query->arg1[len] = '\0';
      out_query->type = CMD_MONGO_UPDATE;
      return 1;
    }
  } else if (strncasecmp(input, "db.count", 8) == 0) {
    out_query->type = CMD_MONGO_COUNT;
    return 1;
  } else if (strncasecmp(input, "EXIT", 4) == 0) {
    out_query->type = CMD_EXIT;
    return 1;
  }

  return 0;
}
