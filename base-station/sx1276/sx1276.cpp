#include "sx1276.hpp"
#include "spi.hpp"

// The following register naming convention follows SX1276 Datasheet chapter 6
#define SX1276REG_OpMode            0x01
#define SX1276REG_FrfMsb            0x06
#define SX1276REG_FrfMid            0x07
#define SX1276REG_FrfLsb            0x08
#define SX1276REG_Ocp               0x0B
#define SX1276REG_ModemConfig1      0x1D
#define SX1276REG_ModemConfig2      0x1E
#define SX1276REG_SymbTimeoutLsb    0x1F
#define SX1276REG_MaxPayloadLength  0x23

// Bandwidth settings, for bits 4-7 of ModemConfig1
#define SX1276_LORA_CODING_RATE_4_5 0x01
#define SX1276_LORA_CODING_RATE_4_6 0x02
#define SX1276_LORA_CODING_RATE_4_7 0x03
#define SX1276_LORA_CODING_RATE_4_8 0x04

#define SX1276_LORA_BW_7800   0x0
#define SX1276_LORA_BW_10400  0x1
#define SX1276_LORA_BW_15600  0x2
#define SX1276_LORA_BW_20800  0x3
#define SX1276_LORA_BW_31250  0x4
#define SX1276_LORA_BW_41700  0x5
#define SX1276_LORA_BW_62500  0x6
#define SX1276_LORA_BW_125000  0x7
#define SX1276_LORA_BW_250000  0x8
#define SX1276_LORA_BW_500000  0x9

#define PASTE(x, y) x ## y
#define BW_TO_SWITCH(number) case number : return PASTE(SX1276_LORA_BW_, number)
#define BW_FR_SWITCH(number) case PASTE(SX1276_LORA_BW_, number) : return number;

inline unsigned BandwidthToBitfield(unsigned bandwidthHz)
{
  switch (bandwidthHz) {
    BW_TO_SWITCH(7800);
    BW_TO_SWITCH(10400);
    BW_TO_SWITCH(15600);
    BW_TO_SWITCH(20800);
    BW_TO_SWITCH(31250);
    BW_TO_SWITCH(41700);
    BW_TO_SWITCH(62500);
    BW_TO_SWITCH(125000);
    BW_TO_SWITCH(250000);
    BW_TO_SWITCH(500000);
    default:
      fprintf(stderr, "Invalid bandwidth %d, falling back to 7.8kHz\n", bandwidthHz);
  }
  return 0;
}

inline unsigned BitfieldToBandwidth(unsigned bitfield)
{
  switch (bitfield) {
    BW_FR_SWITCH(7800);
    BW_FR_SWITCH(10400);
    BW_FR_SWITCH(15600);
    BW_FR_SWITCH(20800);
    BW_FR_SWITCH(31250);
    BW_FR_SWITCH(41700);
    BW_FR_SWITCH(62500);
    BW_FR_SWITCH(125000);
    BW_FR_SWITCH(250000);
    BW_FR_SWITCH(500000);
  }
}

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

// Set everything to a known default state for LoRa communication
void SX1276Radio::ApplyDefaultLoraConfiguration()
{
  uint8_t v;
  // To switch to LoRa mode if we ere in OOK for some reason need to go to sleep mode first : zero 3 lower bits
  spi_->ReadRegister(SX1276REG_OpMode, v);
  spi_->WriteRegister(SX1276REG_OpMode, v & 0xf8);
  usleep(10000);

  // usleeps seem to be empriically needed for bus pirate. TBD for spidev...

  // Switch to LoRa mode in sleep mode, then turn on standby mode
  spi_->WriteRegister(SX1276REG_OpMode, 0x80);
  usleep(100);
  spi_->WriteRegister(SX1276REG_OpMode, 0x81);
  usleep(100);

  spi_->WriteRegister(SX1276REG_MaxPayloadLength, 0x16);
  usleep(100);

  // Switch to maximum current mode (0x1B == 240mA), and enable overcurrent protection
  spi_->WriteRegister(SX1276REG_Ocp, 0x20 | 0x1B);
  usleep(100);

  // Re-read mode and check we set it as expected
  spi_->ReadRegister(SX1276REG_OpMode, v);
  if (v != 0x81) {
    fprintf(stderr, "Unable to enter LoRa standby mode or configure the chip!\n");
    return;
  }

  // spreading factor is E { 2^6 .. 2^ 12 } i.e. 64 .. 4096 chips/symbol
  // with SF6, header MUST be off
  // with BW >= 125 kHZ header should be set on
  // Lets turn the header on, and use 31.25kHz and 4/5 coding and SF9 (512)
  // Symbol timeout BLAH, and CRC
  v = (SX1276_LORA_BW_31250 << 4) | (SX1276_LORA_CODING_RATE_4_5) | 0x1;
  spi_->WriteRegister(SX1276REG_ModemConfig1, v);
  usleep(100);

  // SF9, normal (not continuous) mode, CRC, and upper 2 bits of symbol timeout maximum (1023)
  v = (0x9 << 4) | (0 << 3)| (1 << 2) | 0x3;
  spi_->WriteRegister(SX1276REG_ModemConfig2, v);
  usleep(100);
  v = 0xff;
  spi_->WriteRegister(SX1276REG_SymbTimeoutLsb, v);
  usleep(100);

  // Carrier frequency
  // { F(Xosc) x F } / 2^19
  // Resolution is 61.035 Hz | Xosc=32MHz, default F=0x6c8000 --> 434 MHz
  // AU ISM Band >= 915 MHz
  // Frequency latches when Lsb is written
  // Lets start with 921 MHz...
  uint64_t Frf = 921000000ULL * (1 << 19) / 32000000ULL;
  // printf("Frf=%.8x\n", (uint32_t)Frf);
  v = Frf >> 16;
  spi_->WriteRegister(SX1276REG_FrfMsb, v);
  usleep(100);
  v = Frf >> 8;
  spi_->WriteRegister(SX1276REG_FrfMid, v);
  usleep(100);
  v = Frf & 0xff;
  spi_->WriteRegister(SX1276REG_FrfLsb, v);
  usleep(100);

  spi_->ReadRegister(SX1276REG_FrfMsb, v);
  Frf = uint32_t(v) << 16;
  usleep(100);
  spi_->ReadRegister(SX1276REG_FrfMid, v);
  usleep(100);
  Frf = Frf | (uint32_t(v) << 8);
  usleep(100);
  spi_->ReadRegister(SX1276REG_FrfLsb, v);
  Frf = Frf | (uint32_t(v));

  int64_t fcheck = (32000000ULL * Frf) >> 19;
  printf("Check read Carrier Frequency: %uHz\n", (unsigned)fcheck);
}
