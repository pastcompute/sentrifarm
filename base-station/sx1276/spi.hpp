#ifndef SPI_HPP__
#define SPI_HPP__

#include <stdint.h>

// Abstract interface to SPI.
// Subclassed by BusPirate and SpiDev.
// API assumes registers E 0..7f, with 0x80..ff for write
class SPI
{
public:

  virtual bool IsOpen() const = 0;

  /// Read byte value of single register
  /// @param reg Register E 0..0x7f
  /// @return Data returned by SPI bus transaction
  virtual bool ReadRegister(uint8_t reg, uint8_t& result) = 0;

  /// Write byte value to single register
  /// @param reg Register E 0..0x7f
  /// @return Data returned by SPI bus transaction
  virtual bool WriteRegister(uint8_t reg, uint8_t value) = 0;
};

#endif // SPI_HPP__
