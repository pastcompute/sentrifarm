#include "spidev_spi.hpp"
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

// TODO: abstract fprintf away

SpidevSPI::SpidevSPI()
: fd_(-1)
{
}

SpidevSPI::~SpidevSPI()
{
  close(fd_);
}

bool SpidevSPI::Open(const char *spidev)
{
  assert(fd_ < 0);

  int fd = open(spidev, O_RDWR | O_NOCTTY);
  if (fd == -1) { perror("Unable to open device"); return false; }

  fd_ = fd;
  spidev_ = spidev;
  return true;
}

bool SpidevSPI::ReadRegister(uint8_t reg, uint8_t& result)
{
  return false;
}

uint8_t SpidevSPI::WriteRegister(uint8_t reg, uint8_t value)
{
}
