#ifndef SX1276_HPP__
#define SX1276_HPP__

#include "spi.hpp"
#include <boost/shared_ptr.hpp>

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
class SX1276Radio
{
public:
  /// This radio instance is bound to a particular pre-configured SPI implementation and bus device.
  SX1276Radio(boost::shared_ptr<SPI> spi);
  ~SX1276Radio();

  /// True if the fault flag was set by a prior operation
  bool fault() const { return fault_; }

  int last_rssi() const { return last_rssi_dbm_; }

  /// Query and return the version reported by the SX1276 chip
  int QueryVersion();

  /// Reset the module to our specific configuration.
  /// This should always be called immediately after performing a hardware reset
  bool ApplyDefaultLoraConfiguration();

  /// Send an ASCII message, including zero terminator, truncated by maximum payload length.
  /// No headers or protocol layer are used, this is only a test / beacon function.
  bool SendSimpleMessage(const char *payload);

  /// Wait for a message
  bool ReceiveSimpleMessage(uint8_t buffer[], int& size, int timeout_ms, bool& timeout, bool& crc_error);

  uint8_t StandbyMode();

  float PredictTimeOnAir(const char *payload) const;

private:

  bool WriteRegisterVerify(uint8_t reg, uint8_t value, unsigned intra_delay_us=100);
  bool WriteRegisterVerifyMask(uint8_t reg, uint8_t value, uint8_t mask, unsigned intra_delay_us=100);
  bool ReadRegisterHarder(uint8_t reg, uint8_t& value, unsigned retry=3);

  boost::shared_ptr<SPI> spi_;   ///< Reference to SPI communication instance
  bool fault_;                   ///< True if something went wrong

  uint8_t max_tx_payload_bytes_;
  uint8_t max_rx_payload_bytes_;
  uint32_t frequency_hz_;
  int last_rssi_dbm_;
};

#endif // SX1276_HPP__
