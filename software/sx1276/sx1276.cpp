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
#include "sx1276.hpp"
#include "spi.hpp"
#include "misc.hpp"
#include <string.h>

#include <boost/chrono/time_point.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using boost::chrono::steady_clock;

// IMPORTANT: we seem to be able to transmit OK from the bus pirate (well, 99% of the time)
// IMPORTANT: but, receiving is just not up to the job...

// The following register naming convention follows SX1276 Datasheet chapter 6
#define SX1276REG_Fifo              0x00
#define SX1276REG_OpMode            0x01
#define SX1276REG_FrfMsb            0x06
#define SX1276REG_FrfMid            0x07
#define SX1276REG_FrfLsb            0x08
#define SX1276REG_PaConfig          0x09
#define SX1276REG_PaRamp            0x0A
#define SX1276REG_Ocp               0x0B
#define SX1276REG_Lna               0x0C
#define SX1276REG_FifoAddrPtr       0x0D
#define SX1276REG_FifoTxBaseAddr    0x0E
#define SX1276REG_FifoRxBaseAddr    0x0F
#define SX1276REG_FifoRxCurrentAddr 0x10
#define SX1276REG_IrqFlagsMask      0x11
#define SX1276REG_IrqFlags          0x12
#define SX1276REG_FifoRxNbBytes     0x13
#define SX1276REG_RxHeaderCntValueMsb  0x14
#define SX1276REG_RxHeaderCntValueLsb  0x15
#define SX1276REG_RxPacketCntValueMsb  0x16
#define SX1276REG_RxPacketCntValueLsb  0x17
#define SX1276REG_ModemStat         0x18
#define SX1276REG_PacketSnr         0x19
#define SX1276REG_PacketRssi        0x1A
#define SX1276REG_Rssi              0x1B
#define SX1276REG_ModemConfig1      0x1D
#define SX1276REG_ModemConfig2      0x1E
#define SX1276REG_SymbTimeoutLsb    0x1F
#define SX1276REG_PreambleMSB       0x20
#define SX1276REG_PreambleLSB       0x21
#define SX1276REG_PayloadLength     0x22
#define SX1276REG_MaxPayloadLength  0x23
#define SX1276REG_FifoRxByteAddrPtr 0x25
#define SX1276REG_DioMapping1       0x40
#define SX1276REG_DioMapping2       0x41
#define SX1276REG_PaDac             0x4d

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

#if 1
#define DEBUG(x ...) printf(x)
#else
#define DEBUG(x ...)
#endif

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

SX1276Radio::SX1276Radio(const boost::shared_ptr<SPI>& spi)
: spi_(spi),
  fault_(false),
  max_tx_payload_bytes_(0x80),
  max_rx_payload_bytes_(0x80),
  last_rssi_dbm_(255),
  actual_hz_(0),
  continuousMode_(false),
  continuousSetup_(false),
  preamble_(0x8),
  symbolTimeout_(0x08)
{
  fault_ = !spi_->ReadRegister(0x42, version_);
  ReadCarrier();
}

SX1276Radio::~SX1276Radio()
{
}

/// The Bus Pirate interface seems to be intermittently susceptible to flipping a bit
/// But we seem to be able to cope with that by trying again. Default is 3
/// FIXME: This should probably move to the SPI class
bool SX1276Radio::ReadRegisterHarder(uint8_t reg, uint8_t& value, unsigned retries)
{
  for (unsigned r=0; r < retries; r++) {
    if (spi_->ReadRegister(reg, value)) { return true; }
  }
  fault_ = true;
  PR_ERROR("Retry fail reading register %.02x\n", (int)reg);
  return false;
}

/// The Bus Pirate interface seems to be intermittently susceptible to flipping a bit
/// But we seem to be able to copy with that by trying again
/// FIXME: This should probably move to the SPI class
bool SX1276Radio::WriteRegisterVerify(uint8_t reg, uint8_t value, unsigned intra_delay)
{
  bool ok;
  for (int retry=0; retry < 2; retry++) {
    uint8_t check = ~value;
    ok = spi_->WriteRegister(reg, value);
    if (ok) {
      usleep(intra_delay);
      ok = spi_->ReadRegister(reg, check);
    }
    if (ok && check == value) { return true; }
  }
  PR_ERROR("Failed to verify write of register %.02x\n", (int)reg);
  fault_ = true;
  return false;
}

/// The Bus Pirate interface seems to be intermittently susceptible to flipping a bit
/// But we seem to be able to copy with that by trying again
/// @param mask Bits to modify: ~mask = bits to retain always
/// FIXME: This should probably move to the SPI class
bool SX1276Radio::WriteRegisterVerifyMask(uint8_t reg, uint8_t value, uint8_t mask, unsigned intra_delay)
{
  uint8_t old_value;
  if (!ReadRegisterHarder(reg, old_value, 2)) return false;
  value = (old_value & ~mask) | (value & mask);

  bool ok;
  for (int retry=0; retry < 2; retry++) {
    uint8_t check = ~value;
    ok = spi_->WriteRegister(reg, value);
    if (ok) {
      usleep(intra_delay);
      ok = spi_->ReadRegister(reg, check);
    }
    if (ok && check == value) { return true; }
  }
  PR_ERROR("Failed to verify write of register %.02x\n", (int)reg);
  fault_=true;
  return false;
}

void SX1276Radio::EnterStandby()
{
  WriteRegisterVerify(SX1276REG_OpMode, 0x81);
  continuousSetup_ = false;
  usleep(10000);
}

void SX1276Radio::EnterSleep()
{
  WriteRegisterVerify(SX1276REG_OpMode, 0x80);
  continuousSetup_ = false;
  usleep(10000);
}

bool SX1276Radio::Standby(uint8_t& old_mode)
{
  uint8_t v=0;
  spi_->ReadRegister(SX1276REG_OpMode, v);
  if (!fault_) { EnterStandby(); }
  return !fault_;
}

bool SX1276Radio::Sleep(uint8_t& old_mode)
{
  uint8_t v=0;
  spi_->ReadRegister(SX1276REG_OpMode, v);
  if (!fault_) { EnterSleep(); }
  return !fault_;
}

bool SX1276Radio::ChangeCarrier(uint32_t carrier_hz)
{
  // Carrier frequency
  // { F(Xosc) x F } / 2^19
  // Resolution is 61.035 Hz | Xosc=32MHz, default F=0x6c8000 --> 434 MHz
  // AU ISM Band >= 915 MHz
  // Frequency latches when Lsb is written
  uint8_t v;
  uint64_t Frf = (uint64_t)carrier_hz * (1 << 19) / 32000000ULL;
  v = Frf >> 16;
  WriteRegisterVerify(SX1276REG_FrfMsb, v);
  v = Frf >> 8;
  WriteRegisterVerify(SX1276REG_FrfMid, v);
  v = Frf & 0xff;
  WriteRegisterVerify(SX1276REG_FrfLsb, v);

  ReadCarrier();

  continuousSetup_ = false;
  return fault_;
}

void SX1276Radio::ReadCarrier()
{
  uint8_t v;
  uint64_t Frf = 0;
  ReadRegisterHarder(SX1276REG_FrfMsb, v);
  Frf = uint32_t(v) << 16;
  ReadRegisterHarder(SX1276REG_FrfMid, v);
  Frf = Frf | (uint32_t(v) << 8);
  ReadRegisterHarder(SX1276REG_FrfLsb, v);
  Frf = Frf | (uint32_t(v));

  int64_t actual_hz = (32000000ULL * Frf) >> 19;
  // printf("Check read Carrier Frequency: %uHz\n", (unsigned)fcheck);
  actual_hz_ = actual_hz;
}

bool SX1276Radio::ApplyDefaultLoraConfiguration()
{
  fault_ = false;

  uint8_t v;
  // To switch to LoRa mode if we were in OOK for some reason need to go to sleep mode first : zero 3 lower bits
  spi_->ReadRegister(SX1276REG_OpMode, v);
  WriteRegisterVerify(SX1276REG_OpMode, v & 0xf8);

  // usleep seems to be emprically needed for bus pirate. TBD for spidev...
  usleep(10000);

  // Switch to LoRa mode in sleep mode, then turn on standby mode
  Sleep();
  Standby();

  ReadCarrier();

  // Switch to maximum current mode (0x1B == 240mA), and enable overcurrent protection
  WriteRegisterVerify(SX1276REG_Ocp, (1<<5) | 0x0B); // 0b is default, 1b max, CAUSING ISSUES

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
  v = (0x9 << 4) | (0 << 3)| (1 << 2) | ((symbolTimeout_ >> 8) & 0x03);
  WriteRegisterVerify(SX1276REG_ModemConfig2, v);
  v = symbolTimeout_ & 0xff;
  WriteRegisterVerify(SX1276REG_SymbTimeoutLsb, v);

  // Power (PA)
  // Bit 7 == 0 -- 14dBm max (our inAir9 version)
  // Bit 4..6 --> max power : 10.8 + 0.6 * K  dBm i.e. 10.8, 11.4, 12, 12.6, 13.2, 13.8, 14.4
  // Bit 0..3 --> out power : Pout = pmax - 15 + pout : 0..15 --> -4.2..10.8 ; ... ; -0.6 .. 14

  // Lets start at 12 max, 6dBm out (9) while in the lab

  //v = 0 | (0x2 << 4) | 9;
  //CAUSING ISSUES

#if defined(SX1276_HIGH_POWER)
	// Using the inAir9b:
	fprintf(stderr, "inAir9b High power Mode\n");
	WriteRegisterVerify(SX1276REG_PaConfig, 0xff);
	WriteRegisterVerify(SX1276REG_PaDac, 0x87);
#else
	WriteRegisterVerify(SX1276REG_PaConfig, 0x7f);
	WriteRegisterVerify(SX1276REG_PaDac, 0x84);
#endif
  // TODO: Report node address

  // DIO mapping
  // mapping 1 bits 6..7 DIO0
  // mapping 1 bits 4..5 DIO1
  // mapping 1 bits 2..3 DIO2
  // mapping 1 bits 0..1 DIO3
  // mapping 2 bits 6..7 DIO4
  // mapping 2 bits 4..5 DIO5
  // mapping 2 bits 0 - 1 = multiplex preambledetect 0 = multiplex rssi

  // LED on DIO3 : Valid header : 01
  // Pin header DIO5 : Clk Out: 10
  // Pin header DIO1 : Rx timeout: 00
  // Pin header DIO0 : Tx done: 01

  spi_->ReadRegister(SX1276REG_DioMapping1, v);
  WriteRegisterVerify(SX1276REG_DioMapping1, (v | 0x1) & ~(0x10));

  // WriteRegisterVerify(SX1276REG_DioMapping1, (0x1 << 6) | (0x0 << 4) | (0x1));  CAUSING ISSUES?
  // WriteRegisterVerify(SX1276REG_DioMapping2, (0x2 << 4));                       CAUSING ISSUES?

  continuousSetup_ = false;

  //FIXME: error handling - re-check read after write for everything...
  return !fault_;
}

/// Calcluates the estimated time on air for a given simple payload
/// based on the formulae in the SX1276 datasheet
float SX1276Radio::PredictTimeOnAir(const char *payload) const
{
  unsigned BW = 125000;
  unsigned SF = 9;
  float toa = (6.F+4.25F+8+ceil( (8*(strlen(payload)+1)-4*SF+28+16)/(4*SF))*6.F) * (1 << SF) / BW;
  return toa;
}

float SX1276Radio::PredictTimeOnAir(const void *payload, unsigned len) const
{
  unsigned BW = 125000;
  unsigned SF = 9;
  float toa = (6.F+4.25F+8+ceil( (8*(len+1)-4*SF+28+16)/(4*SF))*6.F) * (1 << SF) / BW;
  return toa;
}


/// Just send raw unframed data i.e. ASCII, zero terminated
bool SX1276Radio::SendSimpleMessage(const char *payload)
{
  char buf[strlen(payload)+1];
  snprintf(buf, sizeof(buf), payload);
  return SendSimpleMessage(payload, strlen(buf)+1);
}

/// Just send raw unframed data
/// Only useful as a beacon but enough to get us started
bool SX1276Radio::SendSimpleMessage(const void *payload, unsigned n)
{
  uint8_t v;
  if (n > 127) { fprintf(stderr, "Message too long; truncated!\n"); }

  continuousSetup_ = false;

  DEBUG("[SX1276][TX] %u\n", n);
  fflush(stdout);
  FILE* f = popen("od -Ax -tx1z -v -w16", "w");
  if (f) { fwrite(payload, n, 1, f); pclose(f); }

  // LoRa Standby
  Standby();

  // Turns out this need to be longer for very short messages
  WriteRegisterVerify(SX1276REG_PreambleLSB, preamble_);

  // Reset TX FIFO
  WriteRegisterVerify(SX1276REG_FifoTxBaseAddr, 0x80);
  WriteRegisterVerify(SX1276REG_FifoAddrPtr, 0x80);
  // Payload length includes zero terminator
  WriteRegisterVerify(SX1276REG_MaxPayloadLength, max_tx_payload_bytes_);
  WriteRegisterVerify(SX1276REG_PayloadLength, n);

  usleep(100);
  uint8_t fifo_pos;
  ReadRegisterHarder(SX1276REG_FifoAddrPtr, fifo_pos);

  for (uint8_t b=0; b < n; b++) {
    spi_->WriteRegister(SX1276REG_Fifo, ((const uint8_t*)payload)[b]); // Note: we cant verify
    usleep(100);
    ReadRegisterHarder(SX1276REG_FifoAddrPtr, v);
    if (v - fifo_pos - 1 != b) {
      fault_ = true;
      PR_ERROR("Failed to write byte to FIFO\n");
      return false;
    }
  }
  ReadRegisterHarder(SX1276REG_FifoAddrPtr, v);
  if (v != 0x80 + n) {
    fault_ = true;
    PR_ERROR("FIFO ptr mismatch, got %.2x expected %.2x\n", (int)v, (int)(0x80+n+1));
    return false;
  }

  // TX mode
  WriteRegisterVerify(SX1276REG_IrqFlagsMask, 0xf7); // write a 1 to IRQ to ignore
  spi_->WriteRegister(SX1276REG_IrqFlags, 0xff); // cant verify; clears on 0xff write
  WriteRegisterVerify(SX1276REG_OpMode, 0x83);
  if (fault_) { PR_ERROR("SPI fault attempting to enter TX mode\n"); spi_->ReadRegister(SX1276REG_IrqFlags, v); return false; }

  // Wait until TX DONE, or timeout
  // We should calculate it; for the moment just wait 1 second

  steady_clock::time_point t0 = steady_clock::now();
  steady_clock::time_point t1 = t0 + boost::chrono::milliseconds(1000); // 1 second is way overkill
  bool done = false;
  do {
    if (!ReadRegisterHarder(SX1276REG_IrqFlags, v, 4)) break;
    if (v & (1 << 3)) {
      done = true;
      break;
    } else {
      usleep(1000);
    }
  } while (steady_clock::now() < t1);
  if (done) {
    DEBUG("[SX1276][TX] fin %u\n", n);
    return true;
  } 
  if (fault_) { PR_ERROR("SPI fault reading IrqFlags register\n"); return false; }
  PR_ERROR("TxDone timeout!\n");
  return false;
}

/// This method blocks up to a given timeout, and wait for a packet.
bool SX1276Radio::ReceiveSimpleMessage(uint8_t buffer[], int& size, int timeout_ms, bool& timeout, bool& crc_error)
{
  uint8_t v;
  uint8_t RX_BASE_ADDR = 0x0;

  if (!continuousMode_ || (continuousMode_ && !continuousSetup_)) {

  // LoRa Standby
  WriteRegisterVerify(SX1276REG_OpMode, 0x81);
  usleep(10000);

  // Reset PA ramp back to default if it was not
  // Why? we are receiving? lWriteRegisterVerify(SX1276REG_PaRamp, 0x09);

  // Setup the LNA:
  // To start with: midpoint gain (gain E G1 (max)..G6 (min)) bits 7-5 and no boost, while we are testing in the lab...
  // Boost, bits 0..1 - 00 = default LNA current, 11 == 150% boost
  WriteRegisterVerify(SX1276REG_FifoTxBaseAddr, (0x4 << 5) | (0x00));


  // For the moment we cant both tx and rx, lots of infrastructural code required if we want that later
  WriteRegisterVerify(SX1276REG_MaxPayloadLength, max_rx_payload_bytes_);
  WriteRegisterVerify(SX1276REG_PayloadLength, max_rx_payload_bytes_);
  // Here is where the datasheet is ambiguous.
  // Page 34: " The register RegRxNbBytes defines the size of the memory location ...
  //            to be written in the event of a successful receive operation"
  //          " The register RegPayloadLength indicates the size of the memory location to be transmitted"
  // WriteRegisterVerify(SX1276REG_FifoRxNbBytes, max_rx_payload_bytes_);

  // Reset RX FIFO
  WriteRegisterVerify(SX1276REG_FifoRxBaseAddr, RX_BASE_ADDR);
  WriteRegisterVerify(SX1276REG_FifoAddrPtr, RX_BASE_ADDR);

  // Note:
  // "RegFifoRxCurrentAddr indicates the location of the last packet received in the FIFO"

  // RX mode
  WriteRegisterVerify(SX1276REG_IrqFlagsMask, 0x0f);
  }

  spi_->WriteRegister(SX1276REG_IrqFlags, 0xff); // cant verify; clears on 0xff write

  if (!continuousMode_) {
    WriteRegisterVerify(SX1276REG_OpMode, 0x86); // RX Single
  } else if (!continuousSetup_) {
    WriteRegisterVerify(SX1276REG_OpMode, 0x85); // RX cont
    continuousSetup_ = true;
  }

  if (fault_) { PR_ERROR("SPI fault attempting to enter RX mode\n"); spi_->ReadRegister(SX1276REG_IrqFlags, v); return false; }

  // Wait until RX DONE, or timeout
  // This is a _user_ polling timeout, because of course there might be no-one transmitting

  steady_clock::time_point t0 = steady_clock::now();
  steady_clock::time_point t1 = t0 + boost::chrono::milliseconds(timeout_ms);

  // User space polling loops are inefficient c/f proper threading wake up (or a kernel driver)
  // But it helps us get working sooner
  uint8_t flags = 0;
  uint8_t stat = 0;
#define TRACE_STATE_CHANGE 0
#if TRACE_STATE_CHANGE
  uint8_t old_stat = 0;
  ReadRegisterHarder(SX1276REG_ModemStat, stat);
  //old_stat = stat;
#endif
  bool done = false;
  bool have_header = false;
  do {
    spi_->TraceSuppressNext(true);
    if (!ReadRegisterHarder(SX1276REG_IrqFlags, flags)) {
      PR_ERROR("SPI fault waiting for packet reading flags.\n");
      return false;
    }
#if TRACE_STATE_CHANGE
    spi_->TraceSuppressNext(true);
    if (!ReadRegisterHarder(SX1276REG_ModemStat, stat)) {
      // this is not critical, so we could probably leave this out and be more resilient
      // but all the error stuff is for the bus pirate anyway
      PR_ERROR("SPI fault waiting for packet reading stat.\n");
      return false;
    }
    if (stat != old_stat) {
      DEBUG("Stat %.2x --> %.2x, flags=%.2x\n", (int)old_stat, (int)stat, (int)flags);
      old_stat = stat;
    }
#endif
    if (flags & (1<<4)) { // valid header
      // dodgy hack : once we started getting a message we rely on the symbol timeout to exit
      have_header = true;
    }
    if (flags & (1 << 6)) { // rx done
      done = true;
      break;
    } else if (flags & (1 << 7)) {
      // symbol timeout: finish
      timeout = true;
      break;
    }
    // still waiting...
#if TRACE_STATE_CHANGE
    usleep(5);
#else
    usleep(100);
#endif
  } while (steady_clock::now() < t1 || have_header); // once we get a valid header dont break until final

  ReadRegisterHarder(SX1276REG_ModemStat, stat);

  last_rssi_dbm_ = 255;
  if (ReadRegisterHarder(SX1276REG_Rssi, v)) { last_rssi_dbm_ = -137 + v; }

  if (!done) {
    // DEBUG("[SX1276][RX] fin flags=%.2x stat=%.2x rssi=%d\n", flags, (int)stat, last_rssi_dbm_);
    return true; // no error, only a timeout or crc
  }

  timeout = false;
  int rssi_packet = 255;
  int snr_packet = -255;
  int coding_rate = 0;
  if (ReadRegisterHarder(SX1276REG_PacketRssi, v)) { rssi_packet = -137 + v; }
  if (ReadRegisterHarder(SX1276REG_PacketSnr, v)) { snr_packet = (v & 0x80 ? (~v + 1) : v) >> 4; } // 2's comp
  if (ReadRegisterHarder(SX1276REG_ModemStat, stat)) {
    coding_rate = stat >> 5;
    switch (coding_rate) {
    case 0x1: coding_rate = 5;
    case 0x2: coding_rate = 6;
    case 0x3: coding_rate = 7;
    case 0x4: coding_rate = 8;
    default: coding_rate = 0;
    }
  }
  uint8_t payloadSizeBytes = 0xff;
  uint16_t headerCount = 0;
  uint16_t packetCount = 0;
  uint8_t byptr = 0;
  ReadRegisterHarder(SX1276REG_FifoRxNbBytes, payloadSizeBytes);
  ReadRegisterHarder(SX1276REG_RxHeaderCntValueMsb, v); headerCount = (uint16_t)v << 8;
  ReadRegisterHarder(SX1276REG_RxHeaderCntValueLsb, v); headerCount |= v;
  ReadRegisterHarder(SX1276REG_RxPacketCntValueMsb, v); packetCount = (uint16_t)v << 8;
  ReadRegisterHarder(SX1276REG_RxPacketCntValueLsb, v); packetCount |= v;
  // Note: SX1276REG_FifoRxByteAddrPtr == last addr written by modem
  ReadRegisterHarder(SX1276REG_FifoRxByteAddrPtr, byptr);

#if LINUX_BEACON
  // NOT WHEN SENT BY ESP8266 / teemnsy!
  payloadSizeBytes--; // DONT KNOW WHY, I THINK FifoRxNbBytes points 1 down
#endif

  DEBUG("[DBUG] ");
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
  printf("%s ", boost::posix_time::to_simple_string(now).c_str());

  DEBUG("RX rssi_pkt=%d ", rssi_packet);
  DEBUG("snr_pkt=%d ", snr_packet);
  DEBUG("stat=%02x ", (unsigned)stat);
  DEBUG("sz=%d ", (unsigned)payloadSizeBytes);
  DEBUG("ptr=%d ", (unsigned)byptr);
  DEBUG("hdrcnt=%d pktcnt=%d\n", (unsigned)headerCount, (unsigned)packetCount);

  if (fault_) { PR_ERROR("SPI fault assessing packet.\n"); return false; }

  // check CRC ...
  if ((flags & (1 << 5)) == 1) {
    PR_ERROR("CRC Error. Packet rssi=%ddBm snr=%d cr=4/%d\n", rssi_packet, snr_packet, coding_rate);
    crc_error = true;
    return true;
  }

  if (fault_) { PR_ERROR("SPI fault processing packet.\n"); return false; }

  if ((int)payloadSizeBytes > size) {
    PR_ERROR("Buffer size %d not large enough for packet size %d!\n", size, (int)payloadSizeBytes);
    return false;
  }

  for (unsigned n=0; n < payloadSizeBytes; n++) {
    uint8_t byte = 0;
    spi_->ReadRegister(SX1276REG_Fifo, byte);
    usleep(100);
    ReadRegisterHarder(SX1276REG_FifoAddrPtr, v); // Note, in the future this extra check should be optional
    if (fault_ || v != RX_BASE_ADDR + v) { PR_ERROR("SPI fault reading packet.\n"); return false; }
    buffer[n] = byte;
  }
  size = payloadSizeBytes;

  return true;

  // Caller should return to standby if going back to sleep
}
