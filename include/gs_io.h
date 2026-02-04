#ifndef GS_IO_H
#define GS_IO_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Abstract IO Interface
 * Allows GS-Lib to write to Files, Memory Buffers, or Network Sockets.
 *
 * To use the library, you must provide an instance of this struct.
 * This guarantees the "Plug and Play" requirement.
 */
typedef struct {
  /**
   * @brief Opaque context pointer (e.g., FILE* or struct buffer*)
   */
  void *context;

  /**
   * @brief Read bytes from the stream
   */
  size_t (*read)(void *ctx, void *buf, size_t size);

  /**
   * @brief Write bytes to the stream
   */
  size_t (*write)(void *ctx, const void *buf, size_t size);

  /**
   * @brief Seek to a specific offset
   */
  int (*seek)(void *ctx, uint64_t offset);

  /**
   * @brief Get current size of the stream/file
   */
  uint64_t (*get_size)(void *ctx);

} gs_io_t;

#endif // GS_IO_H
