#include "sx1276.h"
#include "sx1276reg.h"
#include <SPI.h>

// Somewhat arbitrary starting point
#define DEFAULT_BW_HZ 125000
#define DEFAULT_SPREADING_FACTOR 9
#define DEFAULT_CODING_RATE 6

#define VERBOSE 1

#if VERBOSE
char buf[128]; // dodgy: non-reentrant
#define DEBUG(x ...) { snprintf(buf, sizeof(buf), x); Serial.print(buf); }
#else
#define DEBUG(x ...)
#endif

#undef PASTE
#define PASTE(x, y) x ## y
#define BW_TO_SWITCH(number) case number : return PASTE(SX1276_LORA_BW_, number)
#define BW_FR_SWITCH(number) case PASTE(SX1276_LORA_BW_, number) : return number;

inline byte BandwidthToBitfield(unsigned bandwidth_hz)
{
  switch (bandwidth_hz) {
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
      // Error
      // Note we try and avoid this by validating inputs to methods
      return 0;
  }
}

inline unsigned BitfieldToBandwidth(byte bitfield)
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
    return 0;
  }
}

inline unsigned CodingRateToBitfield(byte coding_rate)
{
  switch(coding_rate) {
    case 5: return SX1276_LORA_CODING_RATE_4_5;
    case 6: return SX1276_LORA_CODING_RATE_4_6;
    case 7: return SX1276_LORA_CODING_RATE_4_7;
    case 8: return SX1276_LORA_CODING_RATE_4_8;
    default:
      // Error
      // Note we try and avoid this by validating inputs to methods
      return 0;
  }
}

SX1276Radio::SX1276Radio(int cs_pin, const SPISettings& spi_settings)
  : cs_pin_(cs_pin),
    spi_settings_(spi_settings),
    symbol_timeout_(0x8),
    preamble_(0x8),
    max_tx_payload_bytes_(0x80),
    bandwidth_hz_(DEFAULT_BW_HZ),
    spreading_factor_(DEFAULT_SPREADING_FACTOR),
    coding_rate_(DEFAULT_CODING_RATE)
{
}

void SX1276Radio::ReadRegister(byte reg, byte& result)
{
  SPI.beginTransaction(spi_settings_);
  digitalWrite(cs_pin_, LOW);
  SPI.transfer(reg);
  result = SPI.transfer(0);
  digitalWrite(cs_pin_, HIGH);
  SPI.endTransaction();
}

// TODO: add a verify version if required (if things dont work)
void SX1276Radio::WriteRegister(byte reg, byte val, byte& result)
{
  SPI.beginTransaction(spi_settings_);
  digitalWrite(cs_pin_, LOW);
  SPI.transfer(reg);
  result = SPI.transfer(val);
  digitalWrite(cs_pin_, HIGH);
  SPI.endTransaction();
  // Needed? delay(10);
}


bool SX1276Radio::Begin()
{
  byte v;

  // Sleep Mode from any unkown mode: clear lower bits
  ReadRegister(SX1276REG_OpMode, v);
  WriteRegister(SX1276REG_OpMode, v & 0xf8);
  delay(10);

  // Sleep
  WriteRegister(SX1276REG_OpMode, 0x80);
  delay(10);

  // LoRa, Standby
  WriteRegister(SX1276REG_OpMode, 0x81);
  delay(10);

  // Switch to maximum current mode (0x1B == 240mA), and enable overcurrent protection
  WriteRegister(SX1276REG_Ocp, (1<<5) | 0x0B); // 0b is default, 1b max

  // Verify operating mode
  ReadRegister(SX1276REG_OpMode, v);
  if (v != 0x81) {
    DEBUG("Unable to enter LoRa mode. v=%.2x\n", v);
    return false;
  }

  // 125kHz, 4/6, explicit header
  v = (BandwidthToBitfield(bandwidth_hz_) << 4) | CodingRateToBitfield(coding_rate_) | 0x0;
  WriteRegister(SX1276REG_ModemConfig1, v);

  // SF9, normal (not continuous) mode, CRC, and upper 2 bits of symbol timeout (maximum i.e. 1023)
  // We use 255, or 255 x (2^9)/125000 or ~1 second
  v = (spreading_factor_ << 4) | (0 << 3)| (1 << 2) | ((symbol_timeout_ >> 8) & 0x03);
  WriteRegister(SX1276REG_ModemConfig2, v);
  v = symbol_timeout_ & 0xff;
  WriteRegister(SX1276REG_SymbTimeoutLsb, v);

  // LED on DIO3 (bits 0..1 of register): Valid header : 01
  // Pin header DIO1 : Rx timeout: 00
  // Pin header DIO0 (bits 6..7) : Tx done: 01
  WriteRegister(SX1276REG_DioMapping1, 0x41);

  // Pin header DIO5 (bits 5-4): Clk Out: 10
  WriteRegister(SX1276REG_DioMapping2, 0x20);

  // Preamble size
  WriteRegister(SX1276REG_PreambleLSB, preamble_);

  return true;
}

void SX1276Radio::GetCarrier(uint32_t& carrier_hz)
{
  uint8_t v;
  uint64_t Frf = 0;
  ReadRegister(SX1276REG_FrfMsb, v);
  Frf = uint32_t(v) << 16;
  ReadRegister(SX1276REG_FrfMid, v);
  Frf = Frf | (uint32_t(v) << 8);
  ReadRegister(SX1276REG_FrfLsb, v);
  Frf = Frf | (uint32_t(v));

  int64_t actual_hz = (32000000ULL * Frf) >> 19;
  carrier_hz = actual_hz;
}

void SX1276Radio::SetCarrier(uint32_t carrier_hz)
{
  // Carrier frequency
  // { F(Xosc) x F } / 2^19
  // Resolution is 61.035 Hz | Xosc=32MHz, default F=0x6c8000 --> 434 MHz
  // AU ISM Band >= 915 MHz
  // Frequency latches when Lsb is written
  byte v;
  uint64_t Frf = (uint64_t)carrier_hz * (1 << 19) / 32000000ULL;
  v = Frf >> 16;
  WriteRegister(SX1276REG_FrfMsb, v);
  v = Frf >> 8;
  WriteRegister(SX1276REG_FrfMid, v);
  v = Frf & 0xff;
  WriteRegister(SX1276REG_FrfLsb, v);
}

/// Calcluates the estimated time on air for a given simple payload
/// based on the formulae in the SX1276 datasheet
// Initial enhancement:
// * use BW & SF from last Begin() or last time they changed
//   so would fail if user ever bypasses this class
// Future enhancement:
// * We could calculate 5 constants only when SF or BW changes
//   Pro: reduces CPU cycles each call
//   Con: need 5 extra fields
// * We could cache past results keyed by length
//   Pro: reduces CPU cycles
//   Con: over time might grow up to 128 (or 255) results x (each time BW or SF changes unless invalidated)
int SX1276Radio::PredictTimeOnAir(byte payload_len) const
{
  // TOA = (6.F+4.25F+8+ceil( (8*payload_len-4*SF+28+16)/(4*SF))*CR) * (1 << SF) / BW;

  // Floating point in the ESP8266 is done in software and is thus expensive

  int N = (payload_len << 3) - 4 * spreading_factor_ + 28 + 16;
  // FUTURE : if header is disabled enabled, k = k - 20;
  int D = 4 * spreading_factor_;
  // FUTURE : if low data rate optimisation enabled, D = D - 2

  // To avoid float, we need to add 1 if there was a remainder
  N = N / D;  if (N % D) { N++; }

  N = N * coding_rate_;
  if (N<0) { N=0; }
  N += 8;

  // N is now set to the number of symbols
  // Add the preamble (preamble + 4.25) then multiply by symbol time Tsym
  // Rsym = BW / 2^Sf  Tsym = 2^sf / BW
  // To avoid float we add them after converting to milliseconds
  int Tpreamble = (1000 * preamble_ + 4250) * (1 << spreading_factor_) / bandwidth_hz_;
  int Tpayload = 1000 * N * (1 << spreading_factor_) / bandwidth_hz_;

  return Tpreamble + Tpayload;
}



bool SX1276Radio::SendMessage(const void *payload, byte len)
{
  if (len > max_tx_payload_bytes_) {
    len = max_tx_payload_bytes_;
    DEBUG("MESSAGE TOO LONG! TRUNCATED\n");
  }

  // LoRa, Standby
  WriteRegister(SX1276REG_OpMode, 0x81);
  delay(10);

  // Reset TX FIFO.
  const byte FIFO_START = 0xff - max_tx_payload_bytes_ + 1;
  WriteRegister(SX1276REG_FifoTxBaseAddr, FIFO_START);
  WriteRegister(SX1276REG_FifoAddrPtr, FIFO_START);
  WriteRegister(SX1276REG_MaxPayloadLength, max_tx_payload_bytes_);
  WriteRegister(SX1276REG_PayloadLength, len);

  // Write payload into FIFO
  // TODO: merge into one SPI transaction
  const byte* p = (const byte*)payload;
  for (byte b=0; b < len; b++) {
    WriteRegister(SX1276REG_Fifo, *p++);
  }

  byte v;
  ReadRegister(SX1276REG_FifoAddrPtr, v);
  if (v != FIFO_START + len) {
    DEBUG("FIFO write pointer mismatch, expected %.2x got %.2x\n", FIFO_START + len, v);
  }

  // TX mode
  WriteRegister(SX1276REG_IrqFlagsMask, 0xf7); // write a 1 to IRQ to ignore
  WriteRegister(SX1276REG_IrqFlags, 0xff); // cant verify; clears on 0xff write
  WriteRegister(SX1276REG_OpMode, 0x83);

  // Wait until TX DONE, or timeout
  // We make the timeout an estimate based on predicted TOA
  bool ok = false;
  int ticks = PredictTimeOnAir(len)  / 10;
  DEBUG("TX TOA TICKS %d\n", ticks);
  do {
    ReadRegister(SX1276REG_IrqFlags, v);
    if (v & (1<<3)) {
      ok = true;
      break;
    }
    if (ticks == 0) { break; }
    delay(10);
    ticks --;
  } while (true);

  if (!ok) {
    DEBUG("TX TIMEOUT!");
    return false;
  }
  // On the way out, we default to staying in LoRa mode
  // the caller can the choose to return to standby
  return true;
}

bool SX1276Radio::ReceiveMessage(byte buffer[], byte& size, int timeout_ms, bool& crc_error)
{

}
