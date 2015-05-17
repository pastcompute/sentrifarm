#include "sx1276.hpp"
#include "spi_factory.hpp"
#include "misc.h"
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>

using boost::shared_ptr;
using boost::format;
using std::cout;

int main(int argc, char* argv[])
{
  if (argc < 2) { fprintf(stderr, "Usage: %s <spidev>", argv[0]); return 1; }

  shared_ptr<SPI> spi = SPIFactory::GetInstance(argv[1]);
  if (!spi) { PR_ERROR("Unable to create SPI device instance\n"); return 1; }

  Misc::UserTraceSettings(spi);

  // TODO work out how to run without powering off / resetting the module

  usleep(100);
  SX1276Radio radio(spi);

  cout << format("SX1276 Version: %.2x\n") % radio.QueryVersion();

  radio.ApplyDefaultLoraConfiguration();

  while (true) {
    if (!radio.SendSimpleMessage("Hello, World!\n")) {
      break;
    }
    usleep(100000);
  }
  return 1;
}
