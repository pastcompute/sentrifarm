#define _BSD_SOURCE
#include "buspirate_binary.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
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

bool bp_bitbang_spi_write_one(int fd, uint8_t reg, uint8_t value)
{
  int8_t reg_mask = reg | 0x80;
  uint8_t cmd[7] = { 0x04, 0, 2, 0, 0, reg_mask, value };
  write(fd, &cmd, 7);

  // 32 kHz?
  usleep(1000); // 1000
  int n=bp_serial_readto(fd, &cmd, 1);
  if (n!=1 || cmd[0] != 0x1 ) { return false; }

  return true;
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

bool bp_setup_serial(int fd, speed_t speed)
{
  struct termios t_opt;

  if (-1 == fcntl(fd, F_SETFL, 0)) { return false; }
  if (-1 == tcgetattr(fd, &t_opt)) { return false; }
  cfsetispeed(&t_opt, speed);
  cfsetospeed(&t_opt, speed);
  t_opt.c_cflag |= (CLOCAL | CREAD);
  t_opt.c_cflag &= ~PARENB;
  t_opt.c_cflag &= ~CSTOPB;
  t_opt.c_cflag &= ~CSIZE;
  t_opt.c_cflag |= CS8;
  t_opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  t_opt.c_iflag &= ~(IXON | IXOFF | IXANY);
  t_opt.c_oflag &= ~OPOST;
  t_opt.c_cc[VMIN] = 0;
  t_opt.c_cc[VTIME] = 1;
  if (-1 == tcflush(fd, TCIFLUSH)) { return false; }
  if (-1 == tcsetattr(fd, TCSANOW, &t_opt)) { return false; }
  return true;
}

bool bp_power_on(int fd)
{
  bool ok;

  printf("POWER ON, AUX:RESET ACTIVE(LOW), CS HIGH\n");

  // Pullups on seems to just get echo of itself
  // Power on, pullups, aux=0, cs=1
  // 0100wxyz - Configure peripherals w=power, x=pull-ups, y=AUX, z=CS
  ok=bp_bitbang_cmd(fd, 0x49);             // 0x4d == 0x40 | (power==0x8) | (pullup=0x4) | (cs)
  if (!ok) { perror("Unable to issue SPI PERIPH"); return false; }

  usleep(6 * 1000);

  // We have aux --> reset...
  printf("POWER ON, AUS:RESET DORMANT(HI), CS HI\n");
  ok=bp_bitbang_cmd(fd, 0x4b);             // 0x4d == 0x40 | (power==0x8) | (pullup=0x4) | aux=2 (cs)
  if (!ok) { perror("Unable to release /RESET (AUX=0)"); return false; }

  usleep(6 * 1000);

  return true;
}

bool bp_spi_config(int fd)
{
  bool ok;
  printf("SPI CONFIG\n");

  // SPI config
  // 1000wxyz - SPI config, w=HiZ/3.3v, x=CKP idle, y=CKE edge, z=SMP sample
  ok=bp_bitbang_cmd(fd, 0x8a);             // 0x80 == 0x8a | (3v3==0x8) | x==idle low(0) | (CKE=a2i==0x2) | middle
  if (!ok) { perror("Unable to issue SPI CONFIG"); return false; }

  printf("SPI SPEED\n");

  // SPI speed = 30 kHz
  // 000=30kHz, 001=125kHz, 010=250kHz, 011=1MHz, 100=2MHz, 101=2.6MHz, 110=4MHz, 111=8MHz
  // struggles to dump at 1M
  ok=bp_bitbang_cmd(fd, 0x62);             // 0x60 == 0x60 | (30k = 000)
  if (!ok) { perror("Unable to issue SPI SPEED"); return false; }

  // Enable CS
  // 0000001x - CS high (1) or low (0)
  //ok = bp_bitbang_cmd(fd, 0x2);           // 0x3 == 0x2 | (ChipSelect)
  // if (!ok) { perror("Unable to activate /CS"); return; }

  return true;
}
