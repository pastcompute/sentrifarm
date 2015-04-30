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

typedef uint8_t byte;

int serial_readto(int fd, void* buf, unsigned bytes)
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
    int n=serial_readto(fd, buf, 5);
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
  int n=serial_readto(fd, buf, 4);
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

bool bp_bitbang_cmd(int fd, byte cmd_byte)
{
  write(fd, &cmd_byte, 1);
  usleep(10);
  int n=serial_readto(fd, &cmd_byte, 1);
  return (n==1 && cmd_byte == 0x1);
}

bool bp_bitbang_spi_read_one(int fd, byte reg, byte *result)
{
  int n;

  byte cmd[6] = { 0x04, 0, 1, 0, 1, reg & 0x7f };
  write(fd, &cmd, 6);

  // 32 kHz?
  usleep(250); // 1000
  n=serial_readto(fd, &cmd, 2);
  if (n!=2 || cmd[0] != 0x1 ) { return false; }

  *result = cmd[1];
  return true;

#if 0
  //  bp_bitbang_cmd(fd, 0x3); // /CS
  if (!bp_bitbang_cmd(fd, 0x4a)) return false;

  byte cmd[2] = { 0x10, 0x5a };
  write(fd, &cmd, 1);
  usleep(1);
  n=serial_readto(fd, &cmd, 1);
  if (n!=1 || cmd[0] != 0x1) { return false; }

  write(fd, &reg, 1);
  cmd[0] = 0xff;
  write(fd, &cmd, 1);

  usleep(1);
  n=serial_readto(fd, &cmd, 2);
  if (n!=2) { return false; }
  *result = cmd[1];

  //  bp_bitbang_cmd(fd, 0x2); // !CS
  return bp_bitbang_cmd(fd, 0x4b);
#endif
}

void sx1276_spi_config(int fd)
{
  bool ok;

  printf("POWER ON, AUX:RESET ACTIVE(LOW), CS HIGH\n");

  // Pullups on seems to just get echo of itself
  // Power on, pullups, aux=0, cs=1
  // 0100wxyz - Configure peripherals w=power, x=pull-ups, y=AUX, z=CS
  ok=bp_bitbang_cmd(fd, 0x49);             // 0x4d == 0x40 | (power==0x8) | (pullup=0x4) | (cs)
  if (!ok) { perror("Unable to issue SPI PERIPH"); return; }

  usleep(6 * 1000);

  // We have aux --> reset...
  printf("POWER ON, AUS:RESET DORMANT(HI), CS HI\n");
  ok=bp_bitbang_cmd(fd, 0x4b);             // 0x4d == 0x40 | (power==0x8) | (pullup=0x4) | aux=2 (cs)
  if (!ok) { perror("Unable to release /RESET (AUX=0)"); return; }

  usleep(6 * 1000);

  printf("SPI CONFIG\n");

  // SPI config
  // 1000wxyz - SPI config, w=HiZ/3.3v, x=CKP idle, y=CKE edge, z=SMP sample
  ok=bp_bitbang_cmd(fd, 0x8a);             // 0x80 == 0x8a | (3v3==0x8) | x==idle low(0) | (CKE=a2i==0x2) | middle
  if (!ok) { perror("Unable to issue SPI CONFIG"); return; }

  printf("SPI SPEED\n");

  // SPI speed = 30 kHz
  // 000=30kHz, 001=125kHz, 010=250kHz, 011=1MHz, 100=2MHz, 101=2.6MHz, 110=4MHz, 111=8MHz 
  // struggles to dump at 1M
  ok=bp_bitbang_cmd(fd, 0x63);             // 0x60 == 0x60 | (30k = 000)
  if (!ok) { perror("Unable to issue SPI SPEED"); return; }

  // Enable CS
  // 0000001x - CS high (1) or low (0) 
  //ok = bp_bitbang_cmd(fd, 0x2);           // 0x3 == 0x2 | (ChipSelect)
  // if (!ok) { perror("Unable to activate /CS"); return; }
}

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
  byte regval[0x71];
  memset(regval, 0x5a, sizeof(regval));
  bool ok = true;
  for (byte r=0; r <= 0x70; r++) {
    byte cmd[6] = { 0x04, 0, 1, 0, 1, r };
    write(fd, cmd, 6);
    // 32 kHz? 125...
    usleep(60 * 0x71); // 1000
    n=serial_readto(fd, cmd, 2);
    if (n!=2 || cmd[0] != 0x1 ) { ok = false; fprintf(stderr, "Dump fault at %.02x\n", r); break; }
    regval[r] = cmd[1];
  }
  if (ok) {
    FILE *f = popen("od -Ax -tx1z -v -w8", "w");
    if (f) { fwrite(regval, 0x71, 1, f); fclose(f); }
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

  if (!(ok = bp_setup_serial(fd, B115200))) { perror("Serial init");  }
  if (ok && !(ok = bp_enable_binary_spi_mode(fd))) { }
  if (ok) {
    printf("BusPirate BitBang SPI mode OK.\n");

    sx1276_spi_config(fd);

    byte version = 0;
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

