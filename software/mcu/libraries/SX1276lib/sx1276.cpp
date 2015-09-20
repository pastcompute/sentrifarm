/*
  Copyright (c) 2015 Andrew McDonnell <bugs@andrewmcdonnell.net>

  This file is part of SentriFarm Radio Relay.

  SentriFarm Radio Relay is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  SentriFarm Radio Relay is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with SentriFarm Radio Relay.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sx1276.h"
#include "sx1276reg.h"
#include <SPI.h>
#if defined(ESP8266)
#include <ets_sys.h>
#else
#define ICACHE_FLASH_ATTR
#endif

// Somewhat arbitrary starting point
#define DEFAULT_BW_HZ 125000
#define DEFAULT_SPREADING_FACTOR 9
#define DEFAULT_CODING_RATE 6

#define VERBOSE 1
#define VERBOSE_W 0
#define VERBOSE_R 0

#ifdef TEENSYDUINO
#define Serial Serial1
#endif

#if VERBOSE
#include <stdio.h>
#define DEBUG(x ...) { char buf[128]; snprintf(buf, sizeof(buf), x); Serial.print(buf); }
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

ICACHE_FLASH_ATTR
SX1276Radio::SX1276Radio(int cs_pin, const SPISettings& spi_settings)
  : cs_pin_(cs_pin),
    spi_settings_(spi_settings),
    symbol_timeout_(366), // in theory 3s, empirically 1.51s @ sf9 and bw 125000 although often, shorter maybe due to esp8266 timekeeping wierdness not accurately recording elapased time
    preamble_(0x8),
    max_tx_payload_bytes_(0x80),
    max_rx_payload_bytes_(0x80),
    bandwidth_hz_(DEFAULT_BW_HZ),
    spreading_factor_(DEFAULT_SPREADING_FACTOR),
    coding_rate_(DEFAULT_CODING_RATE),
    rssi_dbm_(-255),
    rx_snr_db_(-255),
    rx_warm_(false),
    dead_(true)
{
  // Note; we want DEBUG ( Serial) here because this happens before Serial is initialised,
  // and it hangs the ESP8266
  // DEBUG("SX1276: CS pin=%d\n", cs_pin_);
}

//ICACHE_FLASH_ATTR
void SX1276Radio::ReadRegister(byte reg, byte& result)
{
  SPI.beginTransaction(spi_settings_);
  digitalWrite(cs_pin_, LOW);
  SPI.transfer(reg);
  result = SPI.transfer(0);
  digitalWrite(cs_pin_, HIGH);
  SPI.endTransaction();
#if VERBOSE_R
  if (reg != SX1276REG_IrqFlags) {
    DEBUG("[R] %02x --> %02x\n\r", reg, result);
  }
#endif
}

// TODO: add a verify version if required (if things dont work)
//ICACHE_FLASH_ATTR
void SX1276Radio::WriteRegister(byte reg, byte val, byte& result, bool verify)
{
  SPI.beginTransaction(spi_settings_);
  digitalWrite(cs_pin_, LOW);
  SPI.transfer(reg + 0x80);  // Dont forget to set the high bit!
  result = SPI.transfer(val);
  digitalWrite(cs_pin_, HIGH);
  SPI.endTransaction();
#if VERBOSE_W
  DEBUG("[W] %02x <-- %02x --> %02x\n\r", reg, val, result);
#endif
  if (verify) {
    delay(10);
    byte newval = 0;
    ReadRegister(reg, newval);
    if (newval != val) {
      DEBUG("[W] %02x <- %02x failed, got %02x\n\r", reg, val, newval);
    }
  }
}

ICACHE_FLASH_ATTR
byte SX1276Radio::ReadVersion()
{
  byte v;
  ReadRegister(SX1276REG_Version, v);
  return v;
}

ICACHE_FLASH_ATTR
bool SX1276Radio::Begin()
{
  byte v;

  // Sleep Mode from any unkown mode: clear lower bits
  ReadRegister(SX1276REG_OpMode, v);
  WriteRegister(SX1276REG_OpMode, v & 0xf8, true);

  // Sleep
  WriteRegister(SX1276REG_OpMode, 0x80, true);

  // LoRa, Standby
  WriteRegister(SX1276REG_OpMode, 0x81, true);

  // Switch to maximum current mode (0x1B == 240mA), and enable overcurrent protection
  WriteRegister(SX1276REG_Ocp, (1<<5) | 0x0B); // 0b is default, 1b max

  // Verify operating mode
  ReadRegister(SX1276REG_OpMode, v);
  if (v != 0x81) {
    DEBUG("Unable to enter LoRa mode. v=0x%02x\n\r", v);
    return false;
  }
  dead_ = false;

  // IMPORTANT: Testing of 2015-09-13 was accidentally done using 4/5
  // because we forgot to shift the CR. Which also explains why we never get CRC errors!
  // because implicit header could have been on
  // 125kHz, 4/6, explicit header
  v = (BandwidthToBitfield(bandwidth_hz_) << 4) | (CodingRateToBitfield(coding_rate_) << 1) | 0x0;
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

#if defined(SX1276_HIGH_POWER)
  WriteRegister(SX1276REG_PaConfig, 0xff); // inAir9b
  WriteRegister(SX1276REG_PaDac, 0x87);
#else
  // WriteRegister(SX1276REG_PaConfig, 0x7f);
  // Default: WriteRegister(SX1276REG_PaDac, 0x84);
#endif

  // Preamble size
  WriteRegister(SX1276REG_PreambleLSB, preamble_);

  return true;
}

ICACHE_FLASH_ATTR
void SX1276Radio::ReadCarrier(uint32_t& carrier_hz)
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

ICACHE_FLASH_ATTR
void SX1276Radio::SetCarrier(uint32_t carrier_hz)
{
  rx_warm_ = false;
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
ICACHE_FLASH_ATTR
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

ICACHE_FLASH_ATTR
void SX1276Radio::Standby()
{
  WriteRegister(SX1276REG_OpMode, 0x81);
  delay(10);
  rx_warm_ = false;
}

ICACHE_FLASH_ATTR
bool SX1276Radio::TransmitMessage(const void *payload, byte len)
{
  if (len > max_tx_payload_bytes_) {
    len = max_tx_payload_bytes_;
    DEBUG("MESSAGE TOO LONG! TRUNCATED\n\r");
  }

  rx_warm_ = false;
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
    DEBUG("FIFO write pointer mismatch, expected %02x got %02x\n\r", FIFO_START + len, v);
  }

  // TX mode
  WriteRegister(SX1276REG_IrqFlagsMask, 0xf7); // write a 1 to IRQ to ignore
  WriteRegister(SX1276REG_IrqFlags, 0xff); // cant verify; clears on 0xff write
  WriteRegister(SX1276REG_OpMode, 0x83);

  // Wait until TX DONE, or timeout
  // We make the timeout an estimate based on predicted TOA
  bool ok = false;
  int ticks = 1 + PredictTimeOnAir(len)  / 10;
  DEBUG("TX TOA TICKS %d\n\r", ticks);
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

const uint8_t RX_BASE_ADDR = 0x0;

ICACHE_FLASH_ATTR
void SX1276Radio::ReceiveInit()
{
  if (rx_warm_) return;

  // LoRa, Standby
  WriteRegister(SX1276REG_OpMode, 0x81);
  delay(10);

  // Setup the LNA:
  // To start with: midpoint gain (gain E G1 (max)..G6 (min)) bits 7-5 and no boost, while we are testing in the lab...
  // Boost, bits 0..1 - 00 = default LNA current, 11 == 150% boost
  WriteRegister(SX1276REG_FifoTxBaseAddr, (0x4 << 5) | (0x00));
  // WTF? WriteRegister(SX1276REG_Lna, (0x4 << 5) | (0x00));

  WriteRegister(SX1276REG_MaxPayloadLength, max_rx_payload_bytes_);
  WriteRegister(SX1276REG_PayloadLength, max_rx_payload_bytes_);

  // Reset RX FIFO
  WriteRegister(SX1276REG_FifoRxBaseAddr, RX_BASE_ADDR);
  WriteRegister(SX1276REG_FifoAddrPtr, RX_BASE_ADDR);

  // RX mode
  WriteRegister(SX1276REG_IrqFlagsMask, 0x0f);

  rx_warm_ = true;
}

ICACHE_FLASH_ATTR
bool SX1276Radio::ReceiveMessage(byte buffer[], byte size, byte& received, bool& crc_error)
{
  if (size < 1) { return false; }
  if (size < max_rx_payload_bytes_) {
    //DEBUG("BUFFER MAYBE TOO SHORT! DATA POTENTIALLY MAY BE LOST!\n\r");
  }

  // In most use cases we probably want to to this once then stay 'warm'
  ReceiveInit();

  WriteRegister(SX1276REG_IrqFlags, 0xff); // note, this one cant be verified; clears on 0xff write

  WriteRegister(SX1276REG_OpMode, 0x86); // RX Single mode

  // Now we block, until symbol timeout or we get a message
  // Which in practice means polling the IRQ flags
  // and for the purpose of the ESP8266, we need to yield occasionally

  bool done = false;
  bool symbol_timeout = false;
  crc_error = false;
  byte flags = 0;
  do {
    flags = 0;
    ReadRegister(SX1276REG_IrqFlags, flags);
    if (flags & (1 << 4)) {
      // valid header
    }
    if (flags & (1 << 6)) {
      done = true;
      break;
    }
    symbol_timeout = flags & (1 << 7);
    if (!symbol_timeout) { yield(); }
  } while (!symbol_timeout);

  byte v = 0;
  byte stat = 0;

  rssi_dbm_ = -255;
  rx_snr_db_ = -255;
  ReadRegister(SX1276REG_Rssi, v); rssi_dbm_ = -137 + v;

  if (!done) { return false; }

  int rssi_packet = 255;
  int snr_packet = -255;
  int coding_rate = 0;
  ReadRegister(SX1276REG_PacketRssi, v); rssi_packet = -137 + v;
  ReadRegister(SX1276REG_PacketSnr, v); snr_packet = (v & 0x80 ? (~v + 1) : v) >> 4; // 2's comp
  ReadRegister(SX1276REG_ModemStat, stat);
  coding_rate = stat >> 5;
  switch (coding_rate) {
    case 0x1: coding_rate = 5; break;
    case 0x2: coding_rate = 6; break;
    case 0x3: coding_rate = 7; break;
    case 0x4: coding_rate = 8; break;
    default: coding_rate = 0;  break;
  }
  rx_snr_db_ = snr_packet;

  byte payloadSizeBytes = 0xff;
  uint16_t headerCount = 0;
  uint16_t packetCount = 0;
  uint8_t byptr = 0;
  ReadRegister(SX1276REG_FifoRxNbBytes, payloadSizeBytes);
  ReadRegister(SX1276REG_RxHeaderCntValueMsb, v); headerCount = (uint16_t)v << 8;
  ReadRegister(SX1276REG_RxHeaderCntValueLsb, v); headerCount |= v;
  ReadRegister(SX1276REG_RxPacketCntValueMsb, v); packetCount = (uint16_t)v << 8;
  ReadRegister(SX1276REG_RxPacketCntValueLsb, v); packetCount |= v;
  // Note: SX1276REG_FifoRxByteAddrPtr == last addr written by modem
  ReadRegister(SX1276REG_FifoRxByteAddrPtr, byptr);

  payloadSizeBytes--; // DONT KNOW WHY, I THINK FifoRxNbBytes points 1 down

  DEBUG("[DBUG] ");
  DEBUG("RX rssi_pkt=%d ", rssi_packet);
  DEBUG("snr_pkt=%d ", snr_packet);
  DEBUG("stat=%02x ", (unsigned)stat);
  DEBUG("sz=%d ", (unsigned)payloadSizeBytes);
  DEBUG("ptr=%d ", (unsigned)byptr);
  DEBUG("hdrcnt=%d pktcnt=%d\n\r", (unsigned)headerCount, (unsigned)packetCount);

  // check CRC ...
  if ((flags & (1 << 5)) == 1) {
    DEBUG("CRC Error. Packet rssi=%ddBm snr=%d cr=4/%d\n\r", rssi_packet, snr_packet, coding_rate);
    crc_error = true;
    return false;
  }

  if ((int)payloadSizeBytes > size) {
    DEBUG("Buffer size %d not large enough for packet size %d!\n\r", size, (int)payloadSizeBytes);
    return false;
  }

  for (unsigned n=0; n < payloadSizeBytes; n++) {
    ReadRegister(SX1276REG_Fifo, v);
    buffer[n] = v;
  }
  received = payloadSizeBytes;
  return true;
}
