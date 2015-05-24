#ifndef SPI_HPP__
#define SPI_HPP__

#include <stdint.h>

/// Abstract interface to SPI.
/// Subclassed by BusPirate and SpiDev.
/// API assumes byte value registers E 0..7f, with 0x80..ff for write
class SPI
{
public:
  SPI() : fd_(-1), trace_reads_(false), trace_writes_(false) {}

  virtual bool IsOpen() const = 0;

  /// Read byte value of single byte value register
  /// @param reg Register E 0..0x7f
  /// @param result Data returned by SPI bus transaction
  /// @return false on error
  virtual bool ReadRegister(uint8_t reg, uint8_t& result) = 0;

  /// Write byte value to single byte value register
  /// @param reg Register E 0..0x7f, will be or'd with 0x80 for transmission
  /// @return false on error
  virtual bool WriteRegister(uint8_t reg, uint8_t value) = 0;

  inline void TraceReads(bool enabled) { trace_reads_ = enabled; }
  inline void TraceWrites(bool enabled) { trace_writes_ = enabled; }

protected:
  int fd_;
  bool trace_reads_;
  bool trace_writes_;
};

#endif // SPI_HPP__
