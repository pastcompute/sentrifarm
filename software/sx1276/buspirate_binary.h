/*
  Copyright (c) 2015 Andrew McDonnell <bugs@andrewmcdonnell.net>

  This file is part of SentriFarm SX1276 Relay.

  SentriFarm SX1276 Relay is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  SentriFarm Radio Relay is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with SentriFarm Radio Relay.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BUSPIRATE_BINARY_H__
#define BUSPIRATE_BINARY_H__

#include <stdint.h>
#include <stdbool.h>
#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int bp_serial_readto(int fd, void* buf, unsigned bytes);
extern bool bp_bitbang_cmd(int fd, uint8_t cmd_byte);
extern bool bp_bitbang_spi_read_one(int fd, uint8_t reg, uint8_t *result);
extern bool bp_bitbang_spi_write_one(int fd, uint8_t reg, uint8_t value);
extern bool bp_enable_binary_spi_mode(int fd);
extern bool bp_setup_serial(int fd, speed_t speed);
extern bool bp_spi_config(int fd);
extern bool bp_power_on(int fd);
extern bool bp_power_off(int fd);
extern bool bp_cycle_reset(int fd);
extern bool bp_power_cycle(int fd);

#ifdef __cplusplus
}
#endif

#endif // BUSPIRATE_BINARY_H__
