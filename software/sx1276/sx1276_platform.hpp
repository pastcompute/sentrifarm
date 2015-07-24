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
/// @brief Platform specific functions, such as module reset, particular to the SX1276
/// This allows differences like GPIO to be abstracted away from use

#ifndef SX1276_PLATFORM_HPP__
#define SX1276_PLATFORM_HPP__

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

class SPI;

class SX1276Platform : public boost::noncopyable
{
public:
  SX1276Platform();
  virtual ~SX1276Platform();

  static boost::shared_ptr<SX1276Platform> GetInstance(const char *device);

  virtual bool PowerSX1276(bool powered) = 0;
  virtual bool PowerCycleSX1276(bool powered) = 0;
  virtual bool ResetSX1276() = 0;

  virtual boost::shared_ptr<SPI> GetSPI() const = 0;
};

#endif // SX1276_PLATFORM_HPP__
