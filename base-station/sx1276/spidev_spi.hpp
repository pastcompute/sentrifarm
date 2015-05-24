#ifndef SPIDEV_SPI_HPP__
#define SPIDEV_SPI_HPP__

#include "spi.hpp"
#include <string>

class SpidevSPI : public SPI
{
public:
  SpidevSPI();
  virtual ~SpidevSPI();

  bool Open(const char *spidev);
  virtual bool IsOpen() const { return fd_ >= 0; }

  virtual bool ReadRegister(uint8_t reg, uint8_t& result);
  virtual bool WriteRegister(uint8_t reg, uint8_t value);

private:
  bool ConfigureSPI();

  std::string spidev_;
};

#endif // SPIDEV_SPI_HPP
