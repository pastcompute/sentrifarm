#ifndef SPI_FACTORY_HPP__
#define SPI_FACTORY_HPP__

#include <boost/shared_ptr.hpp>

class SPI;

class SPIFactory {
public:
  static boost::shared_ptr<SPI> GetInstance(const char *device);
};

#endif // SPI_FACTORY_HPP__
