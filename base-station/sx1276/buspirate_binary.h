#ifndef BUSPIRATE_BINARY_H__
#define BUSPIRATE_BINARY_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int bp_serial_readto(int fd, void* buf, unsigned bytes);
extern bool bp_bitbang_cmd(int fd, uint8_t cmd_byte);
extern bool bp_bitbang_spi_read_one(int fd, uint8_t reg, uint8_t *result);
extern bool bp_enable_binary_spi_mode(int fd);

#ifdef __cplusplus
}
#endif

#endif // BUSPIRATE_BINARY_H__
