#include "sx1276.hpp"
#include "spi.hpp"

SX1276Radio::SX1276Radio(boost::shared_ptr<SPI> spi)
  : spi_(spi)
{
}

SX1276Radio::~SX1276Radio()
{
}

int SX1276Radio::QueryVersion()
{
  uint8_t value;
  if (spi_->ReadRegister(0x42, value)) { return value; }
  return -1;
}


void SX1276Radio::Reset()
{
}
