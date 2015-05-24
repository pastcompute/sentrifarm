#include "sx1276_platform.hpp"
#include "buspirate_spi.hpp"
#include "buspirate_binary.h"
#include "spidev_spi.hpp"
#include "spi.hpp"
#include <string.h>

using boost::shared_ptr;

class BusPiratePlatform : public SX1276Platform
{
public:
  BusPiratePlatform(const char *device)
  : device_(device)
  {
    spi_.reset(new BusPirateSPI);
  }
  virtual ~BusPiratePlatform() {}

  virtual bool PowerSX1276(bool powered) { return powered ? bp_power_off(spi_->fd_) : bp_power_on(spi_->fd_); }
  virtual bool PowerCycleSX1276(bool powered) { return bp_power_cycle(spi_->fd_); }
  virtual bool ResetSX1276() { return bp_cycle_reset(spi_->fd_); }

  virtual boost::shared_ptr<SPI> GetSPI() const { return spi_; }

private:
  const char *device_;
  shared_ptr<BusPirateSPI> spi_;
};

class Carambola2Platform : public SX1276Platform
{
public:
  Carambola2Platform(const char *device)
  : device_(device)
  {
    shared_ptr<SpidevSPI> spi(new SpidevSPI);
    spi_->Open(device);
  }
  virtual ~Carambola2Platform() {}

  virtual boost::shared_ptr<SPI> GetSPI() const { return spi_; }

  // FIXME : use GPIO to control power / reset
  virtual bool PowerSX1276(bool powered) { return true; }
  virtual bool PowerCycleSX1276(bool powered)  { return true; }
  virtual bool ResetSX1276()  { return true; }

private:
  const char *device_;
  shared_ptr<SpidevSPI> spi_;
};

shared_ptr<SX1276Platform> SX1276Platform::GetInstance(const char *device)
{
  shared_ptr<SX1276Platform> platform;
  // For the time being, use a simple heuristic:
  // if not /dev/spidev then tty for buspirate
  const char *PFX_SPIDEV = "/dev/spidev";
  if (strncmp(device, PFX_SPIDEV, strlen(PFX_SPIDEV))==0) {
    platform.reset(new BusPiratePlatform(device));
  } else {
    platform.reset(new Carambola2Platform(device));
  }
  if (platform->GetSPI()->IsOpen()) { return platform; }
  return shared_ptr<SX1276Platform>();
}

SX1276Platform::SX1276Platform()
{
}

SX1276Platform::~SX1276Platform()
{
}
