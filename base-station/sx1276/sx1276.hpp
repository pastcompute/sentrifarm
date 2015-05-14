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
  SX1276Radio(boost::shared_ptr<SPI> spi);
  ~SX1276Radio();

  bool fault() const;

  int QueryVersion();

  /// Reset the module to our specific configuration.
  /// This should always be called immediately after performing a hardware reset
  void ApplyDefaultLoraConfiguration();

private:
  uint8_t StandbyMode();

  boost::shared_ptr<SPI> spi_;   ///< Reference to SPI communication instance
  bool fault_;
};

#endif // SX1276_HPP__
