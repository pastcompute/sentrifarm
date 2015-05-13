#define _BSD_SOURCE
#include "buspirate_binary.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int bp_serial_readto(int fd, void* buf, unsigned bytes)
{
  int n = bytes;
  int timeout = 0;
  while (n > 0 && timeout < 2) { // was <10 but takes forever first time after powerup
    int er = read(fd, buf, n);
    if (er == -1) { return -1; }
    if (er == 0) { timeout++; }
    n -= er;
  }
  return bytes;
}

bool bp_bitbang_cmd(int fd, uint8_t cmd_byte)
{
  write(fd, &cmd_byte, 1);
  usleep(10);
  int n=bp_serial_readto(fd, &cmd_byte, 1);
  return (n==1 && cmd_byte == 0x1);
}

bool bp_bitbang_spi_read_one(int fd, uint8_t reg, uint8_t *result)
{
  uint8_t reg_mask = reg & 0x7f;
  uint8_t cmd[6] = { 0x04, 0, 1, 0, 1, reg_mask };
  write(fd, &cmd, 6);

  // 32 kHz?
  usleep(1000); // 1000
  int n=bp_serial_readto(fd, &cmd, 2);
  if (n!=2 || cmd[0] != 0x1 ) { return false; }

  *result = cmd[1];
  return true;
}

bool bp_enable_binary_spi_mode(int fd)
{
  const int MAX_TRIES = 25;
  char buf[6]; // big enough to hold any of "SPI1" or "BBIO1". Update this if future modes added.

  // loop up to 25 times, send 0x00 each time and pause briefly for a reply (BBIO1)
  bool ok = false;
  for (int i=0; i<MAX_TRIES; i++) {
    char zero=0;
    if (1 != write(fd, &zero, 1)) { perror("write(fd)"); return false; }
    usleep(1);
    int n=bp_serial_readto(fd, buf, 5);
    if (n < 0) { perror("read(fd)"); return false; }
    // Swallow bogus replies before we tried 20 times
    bool match = strncmp(buf, "BBIO1", 5) == 0;
    if (match) { ok = true; break; }
    if (i > 20 && n > 0 && !match) { fprintf(stderr, "Unable to enter BusPirate binary mode, invalid response: '%s'\n", buf); return false; }
  }
  if (!ok) { fprintf(stderr, "Unable to enter BusPirate binary mode, no valid response.\n"); return false; }

  char bspi = 1;
  if (1 != write(fd, &bspi, 1)) { perror("write(fd)"); return false; }
  usleep(1);
  int n=bp_serial_readto(fd, buf, 4);
  if (strncmp(buf, "SPI1", 4) == 0) { return true; }
  fprintf(stderr, " Unable to enter BusPirate bitbang mode SPI, invalid response: '%s'\n", buf);
  return false;
}
