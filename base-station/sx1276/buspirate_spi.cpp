#include "buspirate_spi.hpp"
#include "buspirate_binary.h"
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

// TODO: abstract fprintf away

BusPirateSPI::BusPirateSPI()
: fd_(-1)
{
}

BusPirateSPI::~BusPirateSPI()
{
  if (fd_ > -1) {
    close(fd_);
  }
}

bool BusPirateSPI::Open(const char *ttydev)
{
  assert(fd_ < 0);

  int fd = open(ttydev, O_RDWR | O_NOCTTY);
  if (fd == -1) { perror("Unable to open device"); return false; }

  fd_ = fd;
  if (ConfigSerial() && EnableBinaryMode() && ConfigSPI()) {
    ttydev_ = ttydev;
    return true;
  }
  close(fd);
  fd_ = -1;
  return false;
}

bool BusPirateSPI::ConfigSerial()
{
  // Standard bus pirate binary mode init
  const int speed = B115200;
  struct termios t_opt;

  if (-1 == fcntl(fd_, F_SETFL, 0)) { return false; }
  if (-1 == tcgetattr(fd_, &t_opt)) { return false; }
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
  if (-1 == tcflush(fd_, TCIFLUSH)) { return false; }
  if (-1 == tcsetattr(fd_, TCSANOW, &t_opt)) { return false; }
  return true;
}

bool BusPirateSPI::EnableBinaryMode()
{
  return bp_enable_binary_spi_mode(fd_);
}

// Presently this function is hard coded to support inAir9 SPI. Config should perhaps be an arg.
bool BusPirateSPI::ConfigSPI()
{
  bool ok;

  printf("BP: POWER ON, AUX:RESET ACTIVE(LOW), CS HIGH\n");

  // Pullups on seems to just get echo of itself
  // Power on, pullups, aux=0, cs=1
  // 0100wxyz - Configure peripherals w=power, x=pull-ups, y=AUX, z=CS
  ok=bp_bitbang_cmd(fd_, 0x49);             // 0x4d == 0x40 | (power==0x8) | (pullup=0x4) | (cs)
  if (!ok) { perror("Unable to issue SPI PERIPH"); return false; }

  usleep(6 * 1000);

  // We have aux --> reset...
  printf("BP: POWER ON, AUS:RESET DORMANT(HI), CS HI\n");
  ok=bp_bitbang_cmd(fd_, 0x4b);             // 0x4d == 0x40 | (power==0x8) | (pullup=0x4) | aux=2 (cs)
  if (!ok) { perror("Unable to release /RESET (AUX=0)"); return false; }

  usleep(6 * 1000);

  printf("SPI CONFIG\n");

  // SPI config
  // 1000wxyz - SPI config, w=HiZ/3.3v, x=CKP idle, y=CKE edge, z=SMP sample
  ok=bp_bitbang_cmd(fd_, 0x8a);             // 0x80 == 0x8a | (3v3==0x8) | x==idle low(0) | (CKE=a2i==0x2) | middle
  if (!ok) { perror("Unable to issue SPI CONFIG"); return false; }

  printf("BP: SPI SPEED\n");

  // SPI speed = 30 kHz
  // 000=30kHz, 001=125kHz, 010=250kHz, 011=1MHz, 100=2MHz, 101=2.6MHz, 110=4MHz, 111=8MHz
  // struggles to dump at 1M
  ok=bp_bitbang_cmd(fd_, 0x63);             // 0x60 == 0x60 | (30k = 000)
  if (!ok) { perror("Unable to issue SPI SPEED"); return false; }

  // Enable CS
  // 0000001x - CS high (1) or low (0)
  //ok = bp_bitbang_cmd(fd, 0x2);           // 0x3 == 0x2 | (ChipSelect)
  // if (!ok) { perror("Unable to activate /CS"); return; }
  return true;
}

bool BusPirateSPI::ReadRegister(uint8_t reg, uint8_t& result)
{
  return bp_bitbang_spi_read_one(fd_, reg, &result);
}

uint8_t BusPirateSPI::WriteRegister(uint8_t reg, uint8_t value)
{
}
