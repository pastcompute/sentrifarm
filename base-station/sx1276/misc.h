#ifndef MISC_H__
#define MISC_H__

#include <boost/shared_ptr.hpp>

class SPI;

#define PR_ERROR(x ...) fprintf(stderr, x)

class Misc {
public:
  static void UserTraceSettings(boost::shared_ptr<SPI>);
};


#endif // MISC_H__
