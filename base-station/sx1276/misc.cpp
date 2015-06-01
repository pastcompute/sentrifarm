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
#include "misc.hpp"
#include "spi.hpp"
#include <string.h>

using boost::shared_ptr;

void Misc::UserTraceSettings(shared_ptr<SPI> spi)
{
  char *p;

  spi->TraceReads( (p=getenv("SPI_TRACE_READ")) && strcmp(p, "yes")==0);
  spi->TraceWrites( (p=getenv("SPI_TRACE_WRITE")) && strcmp(p, "yes")==0);
}

