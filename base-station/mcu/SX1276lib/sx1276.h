#include <Arduino.h>
#include <SPI.h>

// Ideally I'd try and share with the Linux version
// But thats a future Yak shaving task if warranted
// Amongst other things, we skip a lot of the error handling in this simplified version
// Like, we dont try and detect if its plugged in wrong (and dont have to care about Bus Pirate errors, etc)
class SX1276Radio
{
public:
  SX1276Radio(int cs_pin);
  SX1276Radio(int cs_pin, const SPISettings& spi_settings);

  // For the time being we just hard-code the symbol timeout and preamble and everything else in here
  bool Begin();

  void SetCarrier(uint32_t carrier_hz);
  void GetCarrier(uint32_t& carrier_hz);

  bool SendMessage(const void *payload, byte len);

  bool ReceiveMessage(uint8_t buffer[], int& size, int timeout_ms, bool& timeout, bool& crc_error);

private:
  void ReadRegister(byte reg, byte& result);
  void WriteRegister(byte reg, byte val, byte& result);
  inline void WriteRegister(byte reg, byte val) { byte unused; WriteRegister(reg, val, unused); }

  int cs_pin_;
  SPISettings spi_settings_;

  uint16_t symbol_timeout_; // Max: 1023 chips
  uint16_t preamble_;
};
