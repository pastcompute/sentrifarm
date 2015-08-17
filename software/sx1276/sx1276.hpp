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
#ifndef SX1276_HPP__
#define SX1276_HPP__

#include "spi.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

class SPI;

/// Class abstracting the use of an SX1276 chipset LoRa module.
///
/// Given time is at a premium wrt. #hackadayprize2015, this is not (yet) a generic implementation.
/// In particular, the class hard-codes use of LoRa mode, for example.
///
/// Notes:
///
/// V1 of the carambola2 shield doesn't provide power control over the module.
/// We leave 3v3 power control and hardware reset external to this class to simplify things.
class SX1276Radio : boost::noncopyable
{
public:

  /// This radio instance is bound to a particular pre-configured SPI implementation and bus device.
  /// @param spi Reference to SPI object
  SX1276Radio(const boost::shared_ptr<SPI>& spi);
  ~SX1276Radio();

  /// True if the fault flag was set by a prior operation. This is usually because of a SPI error.
  bool fault() const { return fault_; }

  /// Reset the fault() flag
  void reset_fault() { fault_ = false; }

  /// Get version of chip. This value is read in the constructor.
  /// If a SPI error occured at that point, returns -1,
  /// @return version Set to the value of version register of SX1276 chip, expect 0x12 for SX1276.
  int version() const { return version_; }

  /// Return the last known rssi
  /// @return Last RSSI value, set by last call to ReceiveSimpleMessage()
  int last_rssi() const { return last_rssi_dbm_; }

  uint32_t carrier() const { return actual_hz_; }

  /// Reset the module to our specific configuration.
  /// This should always be called immediately after performing a hardware reset
  /// @return true if OK, false if unable to switch into LoRa mode, or if a fault() happened.
  bool ApplyDefaultLoraConfiguration();

  /// Change the carrier frequency.
  /// Note that the actual frequency is subject to rounding resolution, so call carrier() to check
  /// @return true if OK, false if a fault() happened.
  bool ChangeCarrier(uint32_t carrier_hz);

  /// Send an ASCII message, including zero terminator, truncated by maximum payload length.
  /// No headers or protocol layer are used, this is only a test / beacon function.
  /// @param payload Message to send
  /// @return true if OK, false if a fault() happened
  bool SendSimpleMessage(const char *payload);

  /// Send a binary message, truncated by maximum payload length.
  bool SendSimpleMessage(const void *payload, unsigned len);

  /// Wait for a message
  bool ReceiveSimpleMessage(uint8_t buffer[], int& size, int timeout_ms, bool& timeout, bool& crc_error);

	void SetSymbolTimeout(unsigned symbolTimeout) { symbolTimeout_ = symbolTimeout; }
  void SetPreamble(unsigned preamble) { preamble_ = preamble; }
  bool EnableContinuousRx(bool enabled) { continuousMode_ = enabled; }

  /// Revert to LoRa standby mode.
  /// @param old_value Previous mode register value
  /// @return true if OK, false if a fault() happened
  bool Standby(uint8_t& old_value);
  bool Standby() { uint8_t dummy; return Standby(dummy); }

  /// Revert to LoRa sleep mode
  /// @param old_value Previous mode register value
  /// @return true if OK, false if a fault() happened
  bool Sleep(uint8_t& old_value);
  bool Sleep() { uint8_t dummy; return Sleep(dummy); }

  /// Predict time on air for a zero terminated payload.
  /// @param Payload text
  /// @return Time on air, seconds.
  float PredictTimeOnAir(const char *payload) const;
  float PredictTimeOnAir(const void *payload, unsigned len) const;

private:

  void ReadCarrier();
  void EnterStandby();
  void EnterSleep();

  bool WriteRegisterVerify(uint8_t reg, uint8_t value, unsigned intra_delay_us=100);
  bool WriteRegisterVerifyMask(uint8_t reg, uint8_t value, uint8_t mask, unsigned intra_delay_us=100);
  bool ReadRegisterHarder(uint8_t reg, uint8_t& value, unsigned retry=3);

  boost::shared_ptr<SPI> spi_;   ///< Reference to SPI communication instance
  bool fault_;                   ///< True if something went wrong
  uint8_t version_;              ///< Version register value read in constructor
  uint8_t max_tx_payload_bytes_;
  uint8_t max_rx_payload_bytes_;
  int last_rssi_dbm_;            ///< RSSI read during last call to ReceiveSimpleMessage
  uint32_t actual_hz_;           ///< Actual carrier frequency, hz
  bool continuousMode_;          ///< If true then next call to ReceiveSimpleMessage will use continuous mode and not return to standby
  bool continuousSetup_;
	unsigned preamble_;
	unsigned symbolTimeout_;
};

#endif // SX1276_HPP__
