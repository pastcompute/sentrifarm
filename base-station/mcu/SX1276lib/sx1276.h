#include <Arduino.h>
#include <SPI.h>

// Ideally I'd try and share with the Linux version
// But thats a future Yak shaving task if warranted
// Amongst other things, we skip a lot of the error handling in this simplified version
// Like, we dont try and detect if its plugged in wrong (and dont have to care about Bus Pirate errors, etc)

/// This class provides a very simple model of the inAir9 SX1276 LoRa module operating in LoRa mode
/// The model is sufficent to use with MQTT-SN in a point to point environment
/// that doesnt require management or adherence to such things as the LoRa LRSC specification
class SX1276Radio
{
public:
  SX1276Radio(int cs_pin, const SPISettings& spi_settings);

  /// Revert to Standby mode and return all settings to a useful default
  /// The caller might choose to reset the module via GPIO prior to calling this
  /// The defaults are chosen somewhat arbitrarily at this stage, and will be tweaked
  /// to values useful to the authors use case!
  bool Begin();

  /// Read chip version byte
  byte ReadVersion();

  /// Get the currently tuned carrier frequency
  void ReadCarrier(uint32_t& carrier_hz);

  /// Get the maximum allowed payload in bytes
  uint8_t GetMaxPayload() const { return max_tx_payload_bytes_; }

  /// Set the carrier frequency
  void SetCarrier(uint32_t carrier_hz);

  /// Return the SX1276 to LoRa standby mode
  void Standby();

  /// Calcluates the estimated time on air in rounded up milliseconds for a given simple payload
  /// based on the formulae in the SX1276 datasheet
  int PredictTimeOnAir(byte payload_len) const;

  /// Send a message, and block until it is sent
  /// @param payload Message body
  /// @param len Length in bytes.
  /// @note Warning! if len > GetMaxPayload() then message is truncated!
  /// @note Assumes device properly configured, i.e. doesnt sanity check other registers
  /// @return false if retry timeout exceeded. Presently internally set to predicted TOA + a fudge factor
  bool TransmitMessage(const void *payload, byte len);

  /// Wait for a message, block until one is received or a symbol timeout occurs
  /// Useful in simple circumstances; may not perform well in high traffic scenarios
  /// @param buffer Buffer large enough to hold largest expected message
  /// @param size Size of buffer
  /// @note Assumes device properly configured, i.e. doesnt sanity check other registers
  /// @param crc_error Set to true if returned false because of crc not timeout
  /// @return false if timeout or crc error
  bool ReceiveMessage(byte buffer[], byte size, byte& received, bool& crc_error);

private:
  void ReadRegister(byte reg, byte& result);

  void WriteRegister(byte reg, byte val, byte& result, bool verify);
  inline void WriteRegister(byte reg, byte val, bool verify = false) { byte unused; WriteRegister(reg, val, unused, verify); }

  void ReceiveInit();

  // module settings
  int cs_pin_;
  SPISettings spi_settings_;

  // radio settings
  uint16_t symbol_timeout_;      // In datasheet units
  uint16_t preamble_;            // In datasheet units
  uint8_t max_tx_payload_bytes_;
  uint8_t max_rx_payload_bytes_;
  uint32_t bandwidth_hz_;        // calculated
  byte spreading_factor_;        // datasheet units: 6,7,8,9,10,11,12
  byte coding_rate_;             // datasheet units: 5,6,7,8

  // radio status
  int rssi_dbm_;
};
