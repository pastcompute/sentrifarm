#ifndef SPIDEV_SPI_HPP__
#define SPIDEV_SPI_HPP__

#include "spi.hpp"
#include <string>

class SpidevSPI : public SPI
{
public:
  SpidevSPI();
  ~SpidevSPI();

  bool Open(const char *spidev);
  virtual bool IsOpen() const { return fd_ < 0; }

  const char *device() const { return spidev_.c_str(); }

  virtual bool ReadRegister(uint8_t reg, uint8_t& result);
  virtual uint8_t WriteRegister(uint8_t reg, uint8_t value);

private:
  std::string spidev_;
  int fd_;
};

#endif // SPIDEV_SPI_HPP
