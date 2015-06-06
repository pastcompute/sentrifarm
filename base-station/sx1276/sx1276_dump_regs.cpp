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
#include "sx1276.hpp"
#include "sx1276_platform.hpp"
#include "misc.hpp"
#include "util.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <string.h>

using boost::shared_ptr;
using boost::format;
using std::cout;

int main(int argc, char* argv[])
{
  if (argc < 2) { fprintf(stderr, "Usage: %s <spidev>", argv[0]); return 1; }

  shared_ptr<SX1276Platform> platform = SX1276Platform::GetInstance(argv[1]);
  if (!platform) { PR_ERROR("Unable to create platform instance\n"); return 1; }

  shared_ptr<SPI> spi = platform->GetSPI();
  if (!spi) { PR_ERROR("Unable to get SPI instance\n"); return 1; }

  Misc::UserTraceSettings(spi);

  usleep(100);

	const unsigned LAST_REG = 0x70;
	uint8_t values[LAST_REG+1];
	bool ok = true;
	for (unsigned n=0; n <= LAST_REG; n++) {
		ok = ok && spi->ReadRegister(n, values[n]);
	}
	if (!ok) { PR_ERROR("Error reading SPI register. Output is probably wrong.\n"); }
  FILE *f = popen("od -Ax -tx1z -v -w8", "w");
  if (f) { fwrite(values, 0x71, 1, f); pclose(f); }
}
