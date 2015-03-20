char configurable_hostname[256] = "esp8266.example.com";

const char* fr_request_hostname(void)
{
  return configurable_hostname;
}
