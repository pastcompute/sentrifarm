#include "sx1276.hpp"
#include "spi.hpp"
#include "misc.h"
#include <string.h>

#include <boost/chrono/time_point.hpp>
#include <boost/chrono/system_clocks.hpp>

using boost::chrono::steady_clock;

// The following register naming convention follows SX1276 Datasheet chapter 6
#define SX1276REG_Fifo              0x00
#define SX1276REG_OpMode            0x01
#define SX1276REG_FrfMsb            0x06
#define SX1276REG_FrfMid            0x07
#define SX1276REG_FrfLsb            0x08
#define SX1276REG_PaConfig          0x09
#define SX1276REG_Ocp               0x0B
#define SX1276REG_FifoAddrPtr       0x0D
#define SX1276REG_FifoTxBaseAddr    0x0E
#define SX1276REG_IrqFlagsMask      0x11
#define SX1276REG_IrqFlags          0x12
#define SX1276REG_ModemConfig1      0x1D
#define SX1276REG_ModemConfig2      0x1E
#define SX1276REG_SymbTimeoutLsb    0x1F
#define SX1276REG_PayloadLength     0x22
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
  fault_(false),
  max_tx_payload_bytes_(0x80),
  frequency_hz_(918000000)
{
}

SX1276Radio::~SX1276Radio()
{
}

bool SX1276Radio::WriteRegisterVerify(uint8_t reg, uint8_t value, unsigned intra_delay)
{
  if (!spi_->WriteRegister(reg, value)) { fault_=true; return false; }
  usleep(intra_delay);
  uint8_t check;
  if (!spi_->ReadRegister(reg, check)) { fault_=true; return false; }
  if (check != value) { PR_ERROR("Failed to verify write of register %.02x\n", (int)reg); fault_=true; return false; }
  return true;
}


int SX1276Radio::QueryVersion()
{
  uint8_t value = 0xff;
  fault_ = !spi_->ReadRegister(0x42, value);
  return value;
}

uint8_t SX1276Radio::StandbyMode()
{
  uint8_t v=0;
  spi_->ReadRegister(SX1276REG_OpMode, v);
  if (!fault_) { spi_->WriteRegister(SX1276REG_OpMode, 0x81); }
  return v;
}

// Set everything to a known default state for LoRa communication
bool SX1276Radio::ApplyDefaultLoraConfiguration()
{
  fault_ = false;

  uint8_t v;
  // To switch to LoRa mode if we ere in OOK for some reason need to go to sleep mode first : zero 3 lower bits
  spi_->ReadRegister(SX1276REG_OpMode, v);
  WriteRegisterVerify(SX1276REG_OpMode, v & 0xf8);

  // usleep seems to be emprically needed for bus pirate. TBD for spidev...
  usleep(10000);

  // Switch to LoRa mode in sleep mode, then turn on standby mode
  WriteRegisterVerify(SX1276REG_OpMode, 0x80);
  WriteRegisterVerify(SX1276REG_OpMode, 0x81);

  // Maximum payload
  WriteRegisterVerify(SX1276REG_MaxPayloadLength, max_tx_payload_bytes_);

  // Switch to maximum current mode (0x1B == 240mA), and enable overcurrent protection
  WriteRegisterVerify(SX1276REG_Ocp, 0x20 | 0x1B);

  // Re-read operating mode and check we set it as expected
  spi_->ReadRegister(SX1276REG_OpMode, v);
  if (fault_ || v != 0x81) {
    fault_ = true;
    fprintf(stderr, "Unable to enter LoRa standby mode or configure the chip!\n");
    return false;
  }

  // Increasing the spreading factor increases sensitivty at expense of effective data throughput
  // Increasing the bandwidth increases data throughput at expense of sensitivity for same SF

  // spreading factor is E { 2^6 .. 2^ 12 } i.e. 64 .. 4096 chips/symbol
  // with SF6, header MUST be off
  // with BW >= 125 kHZ header should be set on

  // Example: header on, 125kHz BW and 4/6 coding and SF9 (512)
  // We can send 64 byte payload in approx 0.4s + preamble (>= 6+4 symbols, @ 2^9/125000
  // say approx 0.45 seconds for a 8+4 preamble

  // 125kHz, 4/6, explicit header
  v = (SX1276_LORA_BW_125000 << 4) | (SX1276_LORA_CODING_RATE_4_6) | 0x0;
  WriteRegisterVerify(SX1276REG_ModemConfig1, v);

  // SF9, normal (not continuous) mode, CRC, and upper 2 bits of symbol timeout (maximum i.e. 1023)
  // We use 255, or 255 x (2^9)/125000 or ~1 second
  v = (0x9 << 4) | (0 << 3)| (1 << 2) | 0x0;
  WriteRegisterVerify(SX1276REG_ModemConfig2, v);
  v = 0xff;
  WriteRegisterVerify(SX1276REG_SymbTimeoutLsb, v);

  // Carrier frequency
  // { F(Xosc) x F } / 2^19
  // Resolution is 61.035 Hz | Xosc=32MHz, default F=0x6c8000 --> 434 MHz
  // AU ISM Band >= 915 MHz
  // Frequency latches when Lsb is written
  uint64_t Frf = (uint64_t)frequency_hz_ * (1 << 19) / 32000000ULL;
  v = Frf >> 16;
  WriteRegisterVerify(SX1276REG_FrfMsb, v);
  v = Frf >> 8;
  WriteRegisterVerify(SX1276REG_FrfMid, v);
  v = Frf & 0xff;
  WriteRegisterVerify(SX1276REG_FrfLsb, v);

  spi_->ReadRegister(SX1276REG_FrfMsb, v);
  Frf = uint32_t(v) << 16;
  spi_->ReadRegister(SX1276REG_FrfMid, v);
  Frf = Frf | (uint32_t(v) << 8);
  spi_->ReadRegister(SX1276REG_FrfLsb, v);
  Frf = Frf | (uint32_t(v));

  int64_t fcheck = (32000000ULL * Frf) >> 19;
  printf("Check read Carrier Frequency: %uHz\n", (unsigned)fcheck);

  // Power (PA)
  // Bit 7 == 0 -- 14dBm max (our inAir9 version)
  // Bit 4..6 --> max power : 10.8 + 0.6 * K  dBm i.e. 10.8, 11.4, 12, 12.6, 13.2, 13.8, 14.4
  // Bit 0..3 --> out power : Pout = pmax - 15 + pout : 0..15 --> -4.2..10.8 ; ... ; -0.6 .. 14

  // Lets start at 12 max, 6dBm out (9) while in the lab

  v = 0 | (0x2 << 4) | 9;
  WriteRegisterVerify(SX1276REG_PaConfig, v);

  // TODO: Report node address

  //FIXME: error handling - re-check read after write for everything...
  return true;
}

/// Just send raw unframed data i.e. ASCII, zero terminated
/// Only useful as a beacon but enough to get us started
bool SX1276Radio::SendSimpleMessage(const char *payload)
{
  uint8_t v;
  unsigned n = strlen(payload);
  if (n > 126) { fprintf(stderr, "Message too long; truncated!\n"); }

  printf("%.126s", payload);

  unsigned BW = 125000;
  unsigned SF = 9;
  float toa = (6.F+4.25F+8+ceil( (8*(n+1)-4*SF+28+16)/(4*SF))*6.F) * (1 << SF) / BW;
  printf("Predicted time on air: %f\n", toa);

  // LoRa Standby
  WriteRegisterVerify(SX1276REG_OpMode, 0x81);
  usleep(10000);

  // Reset TX FIFO
  WriteRegisterVerify(SX1276REG_FifoTxBaseAddr, 0x80);
  WriteRegisterVerify(SX1276REG_FifoAddrPtr, 0x80);
  // Payload length includes zero terminator
  WriteRegisterVerify(SX1276REG_PayloadLength, n+1);

  for (int b=0; b <= n; b++) {
    spi_->ReadRegister(SX1276REG_FifoAddrPtr, v);
    spi_->WriteRegister(SX1276REG_Fifo, payload[b]); // Note: we dont verify
    usleep(100);
  }
  spi_->ReadRegister(SX1276REG_FifoAddrPtr, v);

  // Read ptr register and compare
  if (v != 0x80 + n + 1) { fprintf(stderr, "FIFO ptr mismatch, got %.2x expected %.2x\n", (int)v, (int)(0x80+n+1)); fault_ = true; return false; }

  // TX mode
  WriteRegisterVerify(SX1276REG_IrqFlagsMask, 0x08);
  spi_->WriteRegister(SX1276REG_IrqFlags, 0xff);
  usleep(100);
  spi_->ReadRegister(SX1276REG_IrqFlags, v);
  usleep(100);
  WriteRegisterVerify(SX1276REG_OpMode, 0x83);
  if (fault_) { fprintf(stderr, "Unable to enter TX mode\n"); spi_->ReadRegister(SX1276REG_IrqFlags, v); return false; }

  // Wait until TX DONE, or timeout
  // We should calculate it; for the moment just wait 1 second

  steady_clock::time_point t0 = steady_clock::now();
  steady_clock::time_point t1 = t0 + boost::chrono::milliseconds(1000); // 1 second is way overkill
  bool done = false;
  do {
    spi_->ReadRegister(SX1276REG_IrqFlags, v);
    if (v & 0x08) {
      usleep(1000);
    } else {
      done = true;
      break;
    }
  } while (steady_clock::now() < t1);
  if (done) {
    //spi_->WriteRegister(SX1276REG_OpMode, 0x81);
    usleep(10000);
    return true;
  }    
  fprintf(stderr, "TxDone timeout!\n");
  return false;
}
