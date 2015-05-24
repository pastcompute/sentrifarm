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
  unsigned inter_msg_delay_us = 200000;
  if (getenv("BEACON_INTERVAL")) { inter_msg_delay_us = atoi(getenv("BEACON_INTERVAL")); }

  Misc::UserTraceSettings(spi);

  // TODO work out how to run without powering off / resetting the module

  usleep(100);
  SX1276Radio radio(spi);

  cout << format("SX1276 Version: %.2x\n") % radio.version();

  radio.ChangeCarrier(918000000);
  radio.ApplyDefaultLoraConfiguration();

  cout << format("Check read Carrier Frequency: %uHz\n") % radio.carrier();


  if (radio.fault()) return 1;

  long total = 0;
  long faultCount = 0;
  while (true) {
    total++;

    bool crc = false;
    uint8_t buffer[128]; memset(buffer, 0, 128);
    bool timeout = false;
    int size = 127;
    bool error = !radio.ReceiveSimpleMessage(buffer, size, 10000, timeout, crc);

    if (crc || error) {
      radio.Standby();
      printf("\n");
      faultCount++;
      PR_ERROR("Fault on receive detected: %ld of %ld\n", faultCount, total);
      platform->ResetSX1276();
      radio.ApplyDefaultLoraConfiguration();
      usleep(500000);
    } else if (timeout) {
      printf("x");
    } else {
      FILE* f = popen("od -Ax -tx1z -v -w16", "w");
      if (f) { fwrite(buffer, size, 1, f); pclose(f); }
      //printf("%-.126s", buffer);
      radio.Standby();
      usleep(inter_msg_delay_us);
    }
  }
  return 1;
}
