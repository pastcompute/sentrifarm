#include "generated/autoconf.h"

char configurable_hostname[256] = "esp8266.example.com";

#if defined(CONFIG_ESP8266_BLOB_LWIP) && CONFIG_ESP8266_BLOB_LWIP

#else

const char* fr_request_hostname(void)
{
  return configurable_hostname;
}

#endif
