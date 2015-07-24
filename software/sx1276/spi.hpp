/*
  Copyright (c) 2015 Andrew McDonnell <bugs@andrewmcdonnell.net>

  This file is part of SentriFarm Radio Relay.

  SentriFarm Radio Relay is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  SentriFarm Radio Relay is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with SentriFarm Radio Relay.  If not, see <http://www.gnu.org/licenses/>.
*/
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
