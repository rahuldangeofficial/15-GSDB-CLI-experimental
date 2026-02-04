#ifndef GS_QUERY_H
#define GS_QUERY_H

#include <stdint.h>

typedef enum {
  CMD_UNKNOWN,
  CMD_PUT,          // PUT x y z t val
  CMD_GET,          // GET x y z t
  CMD_STATS,        // STATS
  CMD_VACUUM,       // VACUUM
  CMD_IMPORT,       // IMPORT <file>
  CMD_EXPORT,       // EXPORT <id> <file>
  CMD_INSERT,       // INSERT "key" <file>
  CMD_FIND,         // FIND "key" (Returns ID)
  CMD_FETCH,        // FETCH "key" <file> (Export to file)
  CMD_MONGO_INSERT, // db.insert({...})
  CMD_MONGO_FIND,   // db.find({...})
  CMD_MONGO_DELETE, // db.delete({...})
  CMD_MONGO_UPDATE, // db.update({...})
  CMD_MONGO_COUNT,  // db.count()
  CMD_EXIT          // EXIT
} gsdb_cmd_type_t;

typedef struct {
  gsdb_cmd_type_t type;
  uint16_t x, y, z, t;
  uint32_t val;
  char arg1[2048]; // Filename or JSON
  char arg2[2048]; // Filename (Export)
} gsdb_query_t;

/**
 * @brief Parse a text command into a query struct
 * @return 1 on success, 0 on format error
 */
int gsdb_parse_command(const char *input, gsdb_query_t *out_query);

#endif // GS_QUERY_H
