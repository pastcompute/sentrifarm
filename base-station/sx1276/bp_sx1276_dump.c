#define _BSD_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <signal.h>

#include "buspirate_binary.h"



bool sx1276_dump(int fd)
{
  printf("DUMP REGS\n");
#if 0
  bool error = false;
  int n;
  byte regval[0x71];
  memset(regval, 0x5a, sizeof(regval));
  byte r;
  for (r=0; r <= 0x70; r++) {
    byte cmd[2];
    cmd[0] = 0x10;
    cmd[1] = r;
    bp_bitbang_cmd(fd, 0x2); // /CS
    write(fd, &cmd, 2);
    usleep(1);
    n=serial_readto(fd, cmd, 2);
    if (n != 2 || cmd[0] != 0x01) { error = true; break; }
    regval[r] = cmd[1];
    bp_bitbang_cmd(fd, 0x3); // !CS
  }
  if (error) { fprintf(stderr, "No/incorrect response reading SPI byte at register %.2x\n", r); }
  else {
    FILE *f = popen("od -Ax -tx1z -v -w8", "w");
    if (f) { fwrite(regval, 0x71, 1, f); fclose(f); }
  }
#endif
  int n;
  uint8_t regval[0x71];
  memset(regval, 0x5a, sizeof(regval));
  bool ok = true;
  for (uint8_t r=0; r <= 0x70; r++) {
    uint8_t cmd[6] = { 0x04, 0, 1, 0, 1, r };
    write(fd, cmd, 6);
    // 32 kHz? 125...
    usleep(60 * 0x71); // 1000
    n=bp_serial_readto(fd, cmd, 2);
    if (n!=2 || cmd[0] != 0x1 ) { ok = false; fprintf(stderr, "Dump fault at %.02x\n", r); break; }
    regval[r] = cmd[1];
  }
  if (ok) {
    FILE *f = popen("od -Ax -tx1z -v -w8", "w");
    if (f) { fwrite(regval, 0x71, 1, f); pclose(f); }
  }
  return ok;

}

int g_fd=-1;

void handler(int sig)
{
  if (g_fd > -1) {
    printf("SIGNAL RESET\n");
    write(g_fd, "\x0", 1); // not sure if this is having effect
    usleep(1);
    write(g_fd, "\xf", 1); // not sure if this is having effect
    usleep(1);
    close(g_fd);
    g_fd = -1;
  }
}

int main(int argc, char *argv[])
{
  const bool reset_on_exit = false;
  const char *port = "/dev/ttyUSB0";
  if (argc > 1) { port = argv[1]; }

	int fd = open(port, O_RDWR | O_NOCTTY);
  if (fd == -1) { perror("Unable to open serial port"); return 1; }

  bool ok = true;

  g_fd = fd;
  signal(SIGINT, handler);

  printf("Entering BusPirate BitBang SPI mode...\n");

  if (!(ok = bp_setup_serial(fd, B115200))) { perror("Serial init");  }
  if (ok && !(ok = bp_enable_binary_spi_mode(fd))) { }
  if (ok) {
    printf("OK.\n");

    bp_spi_config(fd);

		// Without this the first register read sometimes misses
		usleep(1000);

    uint8_t version = 0;
    if (!bp_bitbang_spi_read_one(fd, 0x42, &version)) { perror("Failed to read SX1276 version"); }    else printf("SX1276: Version=%.02x\n", (int)version);

    sx1276_dump(fd);

    usleep(1e6);

    signal(SIGINT, SIG_DFL);

    if (reset_on_exit) {
      write(fd, "\x0", 1); // not sure if this is having effect
      usleep(1);
      write(fd, "\xf", 1); // not sure if this is having effect
      usleep(1);
    }
  }
  close(fd);
  return ok ? 0 : 1;
}

