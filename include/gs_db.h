#ifndef GS_DB_H
#define GS_DB_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque Database Handle
 */
typedef struct gs_db_s gsdb_t;

/**
 * @brief Result Structure for operations
 */
typedef struct {
  int status;         // 0 = Error, 1 = Success
  char message[4096]; // Human readable status
  uint32_t val;       // Numeric result (for GET)
  uint64_t count;     // For STATS
} gsdb_result_t;

/**
 * @brief Open the Database
 */
gsdb_t *gsdb_open(const char *filename);

/**
 * @brief Close the Database
 */
void gsdb_close(gsdb_t *db);

/**
 * @brief Execute a raw string command
 * Example: "PUT 1 2 3 4 100"
 */
gsdb_result_t gsdb_execute(gsdb_t *db, const char *command);

/**
 * @brief Run the Vacuum (Compaction) process
 */
gsdb_result_t gsdb_vacuum(gsdb_t *db);

#endif // GS_DB_H
