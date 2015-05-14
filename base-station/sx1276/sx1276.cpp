#include "sx1276.hpp"
#include "spi.hpp"

// Naming convention follows SX1276 Datasheet chapter 6
#define SX1276REG_OpMode            0x01
#define SX1276REG_Ocp               0x0B
#define SX1276REG_MaxPayloadLength  0x23

SX1276Radio::SX1276Radio(boost::shared_ptr<SPI> spi)
: spi_(spi),
  fault_(false)
{
}

SX1276Radio::~SX1276Radio()
{
}

int SX1276Radio::QueryVersion()
{
  uint8_t value = 0xff;
  fault_ = !spi_->ReadRegister(0x42, value);
  return value;
}

uint8_t SX1276Radio::StandbyMode()
{
  uint8_t v;
  if (fault_ = !spi_->ReadRegister(SX1276REG_OpMode, v)) return 0;
  if (fault_ = !spi_->WriteRegister(SX1276REG_OpMode, 0x81)) return 0;
  return v;
}

void SX1276Radio::ApplyDefaultLoraConfiguration()
{
  uint8_t v;
  // This is a default setting
  // To switch to LoRa mode if we ere in OOK for some reason need to go to sleep mode first : zero 3 lower bits
  spi_->ReadRegister(SX1276REG_OpMode, v);
  spi_->WriteRegister(SX1276REG_OpMode, v & 0xf8);
  usleep(10000);

  // Switch to LoRa mode in sleep mode, then turn on standby mode
  spi_->WriteRegister(SX1276REG_OpMode, 0x80);
  usleep(100);
  spi_->WriteRegister(SX1276REG_OpMode, 0x81);
  usleep(100);

  spi_->WriteRegister(SX1276REG_MaxPayloadLength, 0x16);
  usleep(100);

  // Switch to maximum current mode and enable overcurrent protection
  spi_->WriteRegister(SX1276REG_Ocp, 0x20 | 0x1B);
  usleep(100);

  // Re-read mode and check we set it as expected
  spi_->ReadRegister(SX1276REG_OpMode, v);
  if (v != 0x81) {
    fprintf(stderr, "Unable to enter LoRa standby mode\n");
  }
}
