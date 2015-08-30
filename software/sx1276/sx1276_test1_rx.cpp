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
#include "misc.hpp"
#include "sx1276_platform.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <string.h>

using boost::shared_ptr;
using boost::format;
using std::cout;

inline std::string safe_str(const char *m)
{
  std::string result;
  for (; *m; ) { char c = *m++; result += iscntrl(c) ? '.' : c; }
  return result;
}

int main(int argc, char* argv[])
{
  if (argc < 2) { fprintf(stderr, "Usage: %s <spidev>", argv[0]); return 1; }

  shared_ptr<SX1276Platform> platform = SX1276Platform::GetInstance(argv[1]);
  if (!platform) { PR_ERROR("Unable to create platform instance\n"); return 1; }

  shared_ptr<SPI> spi = platform->GetSPI();
  if (!spi) { PR_ERROR("Unable to get SPI instance\n"); return 1; }

  // Pass a small value in for RTL-SDK spectrum analyser to show up
  int inter_msg_delay_us = 200000;
  if (getenv("BEACON_INTERVAL")) { inter_msg_delay_us = atoi(getenv("BEACON_INTERVAL")); }

  Misc::UserTraceSettings(spi);

  // TODO work out how to run without powering off / resetting the module

  usleep(100);
  SX1276Radio radio(spi);

  cout << format("SX1276 Version: %.2x\n") % radio.version();

  platform->ResetSX1276();

  radio.ChangeCarrier(919000000);
  radio.ApplyDefaultLoraConfiguration();
  radio.SetSymbolTimeout(732);

  cout << format("Check read Carrier Frequency: %uHz\n") % radio.carrier();


  if (radio.fault()) return 1;

  unsigned timeout_ms = 5000;
  if (getenv("TIMEOUT")) { timeout_ms = atoi(getenv("TIMEOUT")); }

  setvbuf(stdout, NULL, _IONBF, 0);

  long total = 0;
  long faultCount = 0;
  char xflop = '.';
  while (true) {
    total++;

    bool crc = false;
    uint8_t buffer[128]; memset(buffer, 0, 128);
    bool timeout = false;
    int size = 127;
    bool error = !radio.ReceiveSimpleMessage(buffer, size, timeout_ms, timeout, crc);
    if (crc || error) {
      radio.Standby();
      printf("\n");
      faultCount++;
      PR_ERROR("Fault on receive detected: %ld of %ld\n", faultCount, total);
      platform->ResetSX1276();
      radio.ChangeCarrier(919000000);
      radio.ApplyDefaultLoraConfiguration();
      usleep(500000);
    } else if (timeout) {
      printf("%c\r", xflop); fflush(stdout);
      switch (xflop) {
        case '.' : xflop = '-'; break;
        case '-' : xflop = '/'; break;
        case '/' : xflop = '|'; break;
        case '|' : xflop = '\\'; break;
        case '\\' : xflop = '-'; break;
        default: xflop = '.'; break;
      }
    } else {
      printf("\r"); fflush(stdout);
      FILE* f = popen("od -Ax -tx1z -v -w32", "w");
      if (f) { fwrite(buffer, size, 1, f); pclose(f); }
      //printf("%-.126s", buffer);
      radio.Standby();
      if (inter_msg_delay_us >= 0) { usleep(inter_msg_delay_us); }
    }
  }
  return 1;
}
