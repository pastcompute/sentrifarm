/// @file
/// @brief Platform specific functions, such as module reset, particular to the SX1276
/// This allows differences like GPIO to be abstracted away from use

#ifndef SX1276_PLATFORM_HPP__
#define SX1276_PLATFORM_HPP__

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

class SPI;

class SX1276Platform : public boost::noncopyable
{
public:
  SX1276Platform();
  virtual ~SX1276Platform();

  static boost::shared_ptr<SX1276Platform> GetInstance(const char *device);

  virtual bool PowerSX1276(bool powered) = 0;
  virtual bool PowerCycleSX1276(bool powered) = 0;
  virtual bool ResetSX1276() = 0;

  virtual boost::shared_ptr<SPI> GetSPI() const = 0;
};

#endif // SX1276_PLATFORM_HPP__
