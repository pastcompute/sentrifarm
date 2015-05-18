#include "spidev_spi.hpp"
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

SpidevSPI::SpidevSPI()
{
}

SpidevSPI::~SpidevSPI()
{
  close(fd_);
}

void SpidevSPI::AssertReset()
{
  // Flip the Reset GPIO here
}

bool SpidevSPI::Open(const char *spidev)
{
  assert(fd_ < 0);

  int fd = open(spidev, O_RDWR | O_NOCTTY);
  if (fd == -1) { perror("Unable to open device"); return false; }

  fd_ = fd;
  if (ConfigureSPI()) {
    spidev_ = spidev;
    return true;
  }
  close(fd);
  fd_ = -1;
  return false;
}

bool SpidevSPI::ConfigureSPI()
{
  __u8	lsb, bits, mode;
  __u32	speed;

  // SPI_IOC_RD_MODE32 can be missing on some systems (like Xubuntu 14.04.2) and present on others (openwrt CC)

  if (ioctl(fd_, SPI_IOC_RD_MODE, &mode) < 0) {
          perror("SPI rd_mode");
          return false;
  }
  if (ioctl(fd_, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
          perror("SPI rd_lsb_fist");
          return false;
  }
  if (ioctl(fd_, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
          perror("SPI bits_per_word");
          return false;
  }
  if (ioctl(fd_, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
          perror("SPI max_speed_hz");
          return false;
  }

  printf("%s: spi mode 0x%x, %d bits %sper word, %d Hz max\n",
          spidev_.c_str(), (int)mode, (int)bits, lsb ? "(lsb first) " : "", speed);
  return true;
}

bool SpidevSPI::ReadRegister(uint8_t reg, uint8_t& result)
{
  struct spi_ioc_transfer xfer[2];
  uint8_t buf[4];
  memset(xfer, 0, sizeof(xfer));
  memset(buf, 0, sizeof(buf));
  buf[0] = reg;
  xfer[0].tx_buf = (unsigned long)buf;
  xfer[0].len = 1;
  xfer[1].rx_buf = (unsigned long)buf;
  xfer[1].len = 1;

  int status = ioctl(fd_, SPI_IOC_MESSAGE(2), xfer);
  if (status < 0) { perror("SPI_IOC_MESSAGE"); return false; }
  if (status != 2) { fprintf(stderr, "SPI [R] status: %d at register %d\n", status, (int)reg); return false; }
  result = buf[0];

  if (trace_reads()) { fprintf(stderr, "[R] %.2x --> %.2x\n", (int)reg, (int)result); }
  return true;
}

bool SpidevSPI::WriteRegister(uint8_t reg, uint8_t value)
{
  struct spi_ioc_transfer xfer[2];
  uint8_t buf[4];
  memset(xfer, 0, sizeof(xfer));
  memset(buf, 0, sizeof(buf));
  buf[0] = reg;
  buf[1] = value;
  xfer[0].tx_buf = (unsigned long)buf;
  xfer[0].len = 2;
  xfer[1].rx_buf = (unsigned long)buf;
  xfer[1].len = 0;

  if (trace_writes()) { fprintf(stderr, "[W] %.2x <-- %.2x\n", (int)reg, (int)value); }

  int status = ioctl(fd_, SPI_IOC_MESSAGE(2), xfer);
  if (status < 0) { perror("SPI_IOC_MESSAGE"); return false; }
  if (status != 2) { fprintf(stderr, "SPI [W] status: %d at register %d\n", status, (int)reg); return false; }
  return true;
}
