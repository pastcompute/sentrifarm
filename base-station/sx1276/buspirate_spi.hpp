#ifndef BUSPIRATE_SPI_HPP__
#define BUSPIRATE_SPI_HPP__

#include "spi.hpp"
#include <string>

/// @note
/// To enter Bus Pirate binary mode takes up to 10 seconds.
/// However if we dont reset the busPirate on exit subsequent calls are much faster.
/// We should have an option to make this an explicit user choice.
/// Presently we by chance leave the system unreset... 
class BusPirateSPI : public SPI
{
public:
  BusPirateSPI();
  ~BusPirateSPI();

  bool Open(const char *ttydev);
  virtual bool IsOpen() const { return fd_ < 0; }

  const char *device() const { return ttydev_.c_str(); }

  virtual bool ReadRegister(uint8_t reg, uint8_t& result);
  virtual uint8_t WriteRegister(uint8_t reg, uint8_t value);

private:
  bool ConfigSerial();
  bool EnableBinaryMode();
  bool ConfigSPI();

  std::string ttydev_;
  int fd_;
};

#endif // BUSPIRATE_SPI_HPP
