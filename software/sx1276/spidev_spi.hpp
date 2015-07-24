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
