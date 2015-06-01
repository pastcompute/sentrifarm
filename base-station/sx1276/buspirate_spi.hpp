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

/// @file
/// @brief Provides BusPirate implementation for SPI
#ifndef BUSPIRATE_SPI_HPP__
#define BUSPIRATE_SPI_HPP__

#include "spi.hpp"
#include "sx1276_platform.hpp"
#include <boost/enable_shared_from_this.hpp>
#include <string>

/// BusPirate binary SPI implementation.
///
/// @note
/// To enter Bus Pirate binary mode takes up to 10 seconds.
/// However if we dont reset the busPirate on exit subsequent calls are much faster.
/// We should have an option to make this an explicit user choice.
/// Presently we by chance leave the system unreset... 
class BusPirateSPI : public SPI
{
public:
  BusPirateSPI();
  virtual ~BusPirateSPI();

  bool Open(const char *ttydev);
  bool Powerup();

  virtual bool IsOpen() const { return fd_ >= 0; }

  virtual bool ReadRegister(uint8_t reg, uint8_t& result);
  virtual bool WriteRegister(uint8_t reg, uint8_t value);

  /// At the moment to simplify implementation of the platform class, we make it a friend for access to the fd
  friend class BusPiratePlatform;

private:
  bool ConfigSerial();
  bool EnableBinaryMode();
  bool ConfigSPI();

  std::string ttydev_;
};

#endif // BUSPIRATE_SPI_HPP
