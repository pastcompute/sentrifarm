#include "misc.h"
#include "spi.hpp"
#include <string.h>

using boost::shared_ptr;

void Misc::UserTraceSettings(shared_ptr<SPI> spi)
{
  char *p;

  spi->TraceReads( (p=getenv("SPI_TRACE_READ")) && strcmp(p, "yes")==0);
  spi->TraceWrites( (p=getenv("SPI_TRACE_WRITE")) && strcmp(p, "yes")==0);
}

