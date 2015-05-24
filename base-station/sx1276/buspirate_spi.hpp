/// @file
/// @brief Provides BusPirate implementation for SPI
#ifndef BUSPIRATE_SPI_HPP__
#define BUSPIRATE_SPI_HPP__

#include "spi.hpp"
#include "sx1276_platform.hpp"
#include <boost/enable_shared_from_this.hpp>
#include <string>

/// BusPirate binary SPI implementation.
///
/// @note
/// To enter Bus Pirate binary mode takes up to 10 seconds.
/// However if we dont reset the busPirate on exit subsequent calls are much faster.
/// We should have an option to make this an explicit user choice.
/// Presently we by chance leave the system unreset... 
class BusPirateSPI : public SPI
{
public:
  BusPirateSPI();
  virtual ~BusPirateSPI();

  bool Open(const char *ttydev);
  bool Powerup();

  virtual bool IsOpen() const { return fd_ < 0; }

  virtual bool ReadRegister(uint8_t reg, uint8_t& result);
  virtual bool WriteRegister(uint8_t reg, uint8_t value);


  /// At the moment to simplify implementation of the platform class, we make it a friend for access to the fd
  friend class BusPiratePlatform;

private:
  bool ConfigSerial();
  bool EnableBinaryMode();
  bool ConfigSPI();

  std::string ttydev_;
};

#endif // BUSPIRATE_SPI_HPP
