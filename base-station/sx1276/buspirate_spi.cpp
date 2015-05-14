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
  return bp_setup_serial(fd_, B115200);
}

bool BusPirateSPI::EnableBinaryMode()
{
  return bp_enable_binary_spi_mode(fd_);
}

// Presently this function is hard coded to support inAir9 SPI. Config should perhaps be an arg.
bool BusPirateSPI::ConfigSPI()
{
  return bp_spi_config(fd_);
}

bool BusPirateSPI::Powerup()
{
  return bp_power_on(fd_);
}

bool BusPirateSPI::ReadRegister(uint8_t reg, uint8_t& result)
{
  bool ok = bp_bitbang_spi_read_one(fd_, reg, &result);
  printf("[R] %.2x --> %.2x\n", (int)reg, (int)result);
  return ok;
}

bool BusPirateSPI::WriteRegister(uint8_t reg, uint8_t value)
{
  uint8_t result = 0;
  printf("[W] %.2x <-- %.2x\n", (int)reg, (int)value);
  return bp_bitbang_spi_write_one(fd_, reg, value, &result);
}
