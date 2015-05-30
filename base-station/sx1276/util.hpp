#ifndef UTIL_HPP__
#define UTIL_HPP__

#include <string>
#include <string.h>

namespace util {

inline std::string buf2str(const void *data, unsigned len)
{
  std::string result;
  const char *p = (const char *)data;
  for (unsigned i=0; i < len; i++) {
    char c = *p++;
    result += iscntrl(c) ? '.' : c;
  }
  return result;
}

// Requires : #define _XOPEN_SOURCE 600
inline std::string safe_perror(int code, const char *prefix)
{
  char buf[96] = "";
  strerror_r(code, buf, sizeof(buf));
  if (strlen(buf)==0) { snprintf(buf, sizeof(buf), "Error code %d", code); }
  return (prefix ? (std::string(prefix) + ": ") : std::string()) + std::string(buf);
}

};

#endif // UTIL_HPP__
