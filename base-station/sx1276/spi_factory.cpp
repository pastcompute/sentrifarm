#include "spi_factory.hpp"
#include "buspirate_spi.hpp"
#include "spidev_spi.hpp"
#include "spi.hpp"
#include <string.h>

using boost::shared_ptr;

shared_ptr<SPI> SPIFactory::GetInstance(const char *device)
{
  // For the time being, use a simple heuristic:
  // if not /dev/spidev then tty for buspirate
  const char *PFX_SPIDEV = "/dev/spidev";
  if (strncmp(device, PFX_SPIDEV, strlen(PFX_SPIDEV))==0) {
    shared_ptr<SpidevSPI> spi(new SpidevSPI);
    if (!spi->Open(device)) { return shared_ptr<SpidevSPI>(); }
    return spi;
  }
  shared_ptr<BusPirateSPI> spi(new BusPirateSPI);
  if (!spi->Open(device)) { return shared_ptr<BusPirateSPI>(); }
  if (!spi->Powerup()) { return shared_ptr<BusPirateSPI>(); }
  return spi;
}
