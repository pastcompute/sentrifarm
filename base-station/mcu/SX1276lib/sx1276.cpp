#include "sx1276.h"
#include "sx1276reg.h"
#include <SPI.h>

#define VERBOSE 1

#if VERBOSE
char buf[128]; // dodgy: non-reentrant
#define DEBUG(x ...) { snprintf(buf, sizeof(buf), x); Serial.print(buf); }
#else
#define DEBUG(x ...)
#endif

SX1276Radio::SX1276Radio(int cs_pin)
  : cs_pin_(cs_pin),
    symbol_timeout_(0x8),
    preamble_(0x8)
{
}

SX1276Radio::SX1276Radio(int cs_pin, const SPISettings& spi_settings)
  : cs_pin_(cs_pin),
    spi_settings_(spi_settings),
    symbol_timeout_(0x8),
    preamble_(0x8)
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
  v = (SX1276_LORA_BW_125000 << 4) | (SX1276_LORA_CODING_RATE_4_6) | 0x0;
  WriteRegister(SX1276REG_ModemConfig1, v);

  // SF9, normal (not continuous) mode, CRC, and upper 2 bits of symbol timeout (maximum i.e. 1023)
  // We use 255, or 255 x (2^9)/125000 or ~1 second
  v = (0x9 << 4) | (0 << 3)| (1 << 2) | ((symbol_timeout_ >> 8) & 0x03);
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

bool SX1276Radio::SendMessage(const void *payload, byte len)
{
  int timeout_ms = 1000;
  const byte max_tx_payload_bytes_ = 0x80;
  byte v;
  if (len > max_tx_payload_bytes_) {
    DEBUG("MESSAGE TOO LONG! TRUNCATED\n");
  }

  // LoRa, Standby
  WriteRegister(SX1276REG_OpMode, 0x81);
  delay(10);

  // Reset TX FIFO
  const byte FIFO_START = 0x80;
  WriteRegister(SX1276REG_FifoTxBaseAddr, FIFO_START);
  WriteRegister(SX1276REG_FifoAddrPtr, FIFO_START);
  WriteRegister(SX1276REG_MaxPayloadLength, max_tx_payload_bytes_);
  WriteRegister(SX1276REG_PayloadLength, len);

  // Write payload into FIFO
  // TODO: merge into one SPI transaction
  if (len > max_tx_payload_bytes_) { len = max_tx_payload_bytes_; }
  const byte* p = (const byte*)payload;
  for (byte b=0; b < len; b++) {
    WriteRegister(SX1276REG_Fifo, *p++);
  }

  ReadRegister(SX1276REG_FifoAddrPtr, v);
  if (v != FIFO_START + len) {
    DEBUG("FIFO write pointer mismatch, expected %.2x got %.2x\n", FIFO_START + len, v);
  }

  // TX mode
  WriteRegister(SX1276REG_IrqFlagsMask, 0xf7); // write a 1 to IRQ to ignore
  WriteRegister(SX1276REG_IrqFlags, 0xff); // cant verify; clears on 0xff write
  WriteRegister(SX1276REG_OpMode, 0x83);

  // Wait until TX DONE, or timeout
  // We should calculate it based on predicted TOA; for the moment just wait 1 second
  bool ok = false;
  int ticks = timeout_ms  / 10;
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
  return true;
}

bool SX1276Radio::ReceiveMessage(uint8_t buffer[], int& size, int timeout_ms, bool& timeout, bool& crc_error)
{

}
