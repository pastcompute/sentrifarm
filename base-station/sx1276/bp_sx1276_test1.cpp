#include "sx1276.hpp"
#include "buspirate_spi.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>

using boost::shared_ptr;
using boost::format;
using std::cout;

int main(int argc, char* argv[])
{
  if (argc < 2) { fprintf(stderr, "Usage: %s <spidev>", argv[0]); return 1; }

  shared_ptr<BusPirateSPI> spi(new BusPirateSPI);
  if (!spi->Open(argv[1])) { return 1; }
  if (!spi->Powerup()) { return 1; }

  // TODO work out how to run without powering off / resetting the module

  usleep(100);

  SX1276Radio radio(spi);

  cout << format("Version: %.2x\n") % radio.QueryVersion();

  radio.ApplyDefaultLoraConfiguration();
}
